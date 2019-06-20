#pragma once

#if STORM_GC == STORM_GC_SMM

#include "Arena.h"
#include "Config.h"
#include "Format.h"
#include "Scanner.h"
#include "Gc/MemorySummary.h"
#include <vector>

namespace storm {
	namespace smm {

		/**
		 * Storage for non-moving objects.
		 *
		 * We assume that there will be a relatively small number of allocations here, and as such,
		 * it is not as efficient as moving objects. At the moment, objects are considered a member
		 * of all generations, which causes slightly larger scanning overheads but potentially
		 * faster finalization. At the very least it simplifies the implementation, which outweights
		 * the performance benefits under the assumption that there will be very few allocations in
		 * this pool. We are using a mark-and-sweep approach for this chunk.
		 *
		 * Furthermore, allocations in this pool will have additional headers compared with regular
		 * allocations in order to store necessary book-keeping information, and they require a
		 * slower allocation protocol compared to regular allocations.
		 *
		 * Finally, we don't support "too large" allocations. We only request memory in the chunk
		 * size supported by the underlying VMAlloc, and don't attempt to accommodate space for
		 * allocations larger than that. Large allocations would require additional effort in order
		 * to ensure that fragmentation stays within reasonable limits, and otherwise slow the
		 * system down. If large objects need to be accessible via a non-moving reference, then a
		 * smaller proxy-object can be statically allocated and refer to the larger allocation to
		 * work around the problem.
		 *
		 * We assume that an ArenaTicket is acquired when using this class.
		 */
		class Nonmoving {
		public:
			// Create the chunk, associating it with an arena.
			Nonmoving(Arena &arena);

			// Destroy.
			~Nonmoving();

			// Owning arena.
			Arena &arena;

			// Allocate an object of the given type. Returns a properly initialized client pointer.
			void *alloc(ArenaTicket &ticket, const GcType *type);

			// Free an allocation (expected to be called from inside the GC).
			void free(ArenaTicket &ticket, void *mem);

			// Run finalizers for all objects in here. Assumed to be called before destruction.
			void runFinalizers();

			// Get an address set initialized to a suitable range for us (we assume there are few
			// enough objects so that one is enough).
			template <class AddrSet>
			AddrSet addrSet() const {
				return AddrSet(memMin, memMax);
			}

			/**
			 * Object traversal.
			 */

			// Traverse all objects in this block. Calls Fn for each object, passing client pointers
			// to the function.
			template <class Fn>
			void traverse(Fn fn) {
				for (size_t i = 0; i < chunks.size(); i++)
					chunks[i]->traverse(fn);
			}

			// Scan all objects.
			template <class Scanner>
			typename Scanner::Result scan(typename Scanner::Source &source) {
				typename Scanner::Result result = typename Scanner::Result();
				for (size_t i = 0; i < chunks.size(); i++) {
					result = chunks[i]->scan<Scanner>(source);
					if (result != typename Scanner::Result())
						break;
				}
				return result;
			}

			// Scan all pinned objects.
			template <class Scanner>
			typename Scanner::Result scanPinned(const PinnedSet &pinned, typename Scanner::Source &source) {
				typename Scanner::Result result = typename Scanner::Result();
				for (size_t i = 0; i < chunks.size(); i++) {
					Chunk *chunk = chunks[i];
					if (!pinned.has(chunk, chunk->mem(chunk->size)))
						break;

					result = chunk->scanPinned<Scanner>(pinned, source);
					if (result != typename Scanner::Result())
						break;
				}
				return result;
			}

			// Scan marked objects and sweep unmarked objects.
			template <class Scanner>
			typename Scanner::Result scanSweep(typename Scanner::Source &source) {
				typename Scanner::Result result = typename Scanner::Result();
				for (size_t i = 0; i < chunks.size(); i++) {
					result = chunks[i]->scanSweep<Scanner>(source);
					if (result != typename Scanner::Result())
						break;
				}
				return result;
			}

			// Sweep unmarked objects.
			void sweep(ArenaTicket &ticket);


			/**
			 * Debugging.
			 */

			// Fill a memory summary with information.
			void fillSummary(MemorySummary &summary) const;

			// Verify the integrity of the nonmoving allocations.
			void dbg_verify();

			// Output a summary.
			void dbg_dump();

		public:
			// Header for a single nonmoving allocation. The data managed by the format starts at the
			// end of this object, and client pointers point further ahead.
			struct Header {
				// Get the pointer to the allocation as the object format code sees it.
				inline fmt::Obj *object() {
					byte *data = (byte *)this;
					return (fmt::Obj *)(data + sizeof(Header));
				}

				// Compute a Header pointer from an Obj pointer.
				static inline Header *fromObject(fmt::Obj *obj) {
					byte *data = (byte *)obj;
					return (Header *)(data - sizeof(Header));
				}

				// Compute a header pointer from a client pointer.
				static inline Header *fromClient(void *client) {
					return fromObject(fmt::fromClient(client));
				}

				enum {
					// This allocation describes a piece of free space.
					fFree = 0x01,

					// This allocation is marked as used during a GC phase.
					fMarked = 0x02,

					// Are we traversing this object right now? This is used to avoid cyclic scanning of a single object.
					fTraversing = 0x04,

					// This allocation was deemed unreachable, but must be finalized before it can be reclaimed.
					fFinalize = 0x08,
				};

				// Data on this object. The 8 low bits are reserved for flags, while the remaining
				// 24+ bits are used to indicate size.
				size_t data;

				// Get/set flags.
				inline bool hasFlag(size_t flag) const {
					return (data & flag) != 0;
				}
				inline void setFlag(size_t flag) {
					data |= flag;
				}
				inline void clearFlag(size_t flag) {
					data &= ~flag;
				}

				// Get/set size.
				inline size_t size() const {
					return data >> 8;
				}
				inline void size(size_t s) {
					data &= 0xFF;
					data |= s << 8;
				}

				// Initialize this object as free with the specified size.
				inline void makeFree(size_t sz) {
					data = fFree;
					size(sz);
				}
			};

		private:
			// A chunk of nonmoving allocations. Allocated as a header in the actual allocation.
			class Chunk {
			public:
				// Create, with a given size. The size excludes the header.
				Chunk(size_t size);

				// The size of this chunk, excluding the header.
				const size_t size;

				// Last allocation (offset). We continue searching from here to avoid having to
				// traverse the entire chunk each time.
				size_t lastAlloc;

				// Get a smm::Chunk describing this chunk. Useful for deallocation!
				inline smm::Chunk chunk() {
					return smm::Chunk(this, size + sizeof(Chunk));
				}

				// Create a new chunk from an smm::Chunk.
				static inline Chunk *create(smm::Chunk chunk) {
					return new (chunk.at) Chunk(chunk.size - sizeof(Chunk));
				}

				// Get a particular byte.
				void *mem(size_t offset) {
					return (byte *)this + sizeof(Chunk) + offset;
				}
				const void *mem(size_t offset) const {
					return (const byte *)this + sizeof(Chunk) + offset;
				}

				// Get a Header at a specific offset.
				Header *header(size_t offset) {
					return (Header *)mem(offset);
				}
				const Header *header(size_t offset) const {
					return (const Header *)mem(offset);
				}

				// Compute the offset of a header.
				size_t offset(Header *header) {
					byte *q = (byte *)header;
					byte *start = (byte *)this + sizeof(Chunk);
					return q - start;
				}

				// Allocate memory in this chunk. Assumes 'lock' is held, and arranges for it to be
				// released if the reservation succeeds.
				fmt::Obj *alloc(size_t size);

				// Free memory from this chunk. Assumes that the pointer was previously allocated
				// with 'alloc'. Expected to be called from inside the GC, and not explicitly from
				// client code.
				void free(fmt::Obj *obj);

				// Run all finalizers in this block.
				void runFinalizers();

				// Traverse objects in here.
				template <class Fn>
				void traverse(Fn fn);

				// Scan objects.
				template <class Scanner>
				typename Scanner::Result scan(typename Scanner::Source &source);

				// Scan pinned objects, update the mark bit accordingly.
				template <class Scanner>
				typename Scanner::Result scanPinned(const PinnedSet &pinned, typename Scanner::Source &source);

				// Scan objects while sweeping.
				template <class Scanner>
				typename Scanner::Result scanSweep(typename Scanner::Source &source);

				// Fill a memory summary with information about this chunk.
				void fillSummary(MemorySummary &summary) const;

				// Verify the integrity of this chunk.
				void dbg_verify();

				// Output a summary of this chunk.
				void dbg_dump();

			private:
				// Reserve an object inside the specified address range.
				fmt::Obj *allocRange(size_t size, size_t from, size_t to);

				// Merge this block, which is assumed to be free, with any free blocks directly ahead of it.
				void mergeFree(Header *start);
			};

			// Note: Compares with the *end* of chunks, so that lower_bound returns the interesting
			// element directly, rather than us having to step back one step each time.
			struct PtrCompare {
				inline bool operator ()(const Chunk *a, const void *ptr) const {
					return size_t(a->mem(a->size)) < size_t(ptr);
				}

				inline bool operator ()(const Chunk *a, const Chunk *b) const {
					return size_t(a) < size_t(b);
				}

			};

			// All chunks managed by us. Each one is generally the size of one page from VMAlloc,
			// but don't assume that. Sorted by addresses of the chunks.
			typedef vector<Chunk *> ChunkList;
			ChunkList chunks;

			// Last chunk used. The next allocation will be re-tried there since it is likely to fit
			// the next allocation as well!
			size_t lastChunk;

			// Min- and max addresses.
			size_t memMin, memMax;

			// Allocate memory for an allocation, but don't initialize it. Returns 'null' on failure.
			void *allocMem(size_t size);

			// Allocate another chunk. Returns the ID of the newly allocated chunk, or an non-valid
			// ID if the allocation failed.
			size_t allocChunk();
		};


		template <class Fn>
		void Nonmoving::Chunk::traverse(Fn fn) {
			for (size_t i = 0; i < size; i += header(i)->size() + sizeof(Header)) {
				Header *h = header(i);
				if (h->hasFlag(Header::fFree | Header::fFinalize))
					continue;

				fn(fmt::toClient(h->object()));
			}
		}

		template <class Scanner>
		typename Scanner::Result Nonmoving::Chunk::scan(typename Scanner::Source &source) {
			typename Scanner::Result result = typename Scanner::Result();
			for (size_t i = 0; i < size; i += header(i)->size() + sizeof(Header)) {
				Header *h = header(i);
				if (h->hasFlag(Header::fFree | Header::fFinalize))
					continue;

				void *obj = fmt::toClient(h->object());
				result = fmt::Scan<Scanner>::objects(source, obj, fmt::skip(obj));
				if (result != typename Scanner::Result())
					return result;
			}
			return result;
		}

		template <class Scanner>
		typename Scanner::Result Nonmoving::Chunk::scanPinned(const PinnedSet &pinned, typename Scanner::Source &source) {
			typename Scanner::Result result = typename Scanner::Result();
			for (size_t i = 0; i < size; i += header(i)->size() + sizeof(Header)) {
				Header *h = header(i);
				if (h->hasFlag(Header::fFree | Header::fFinalize))
					continue;

				h->clearFlag(Header::fTraversing);

				void *obj = fmt::toClient(h->object());
				void *end = fmt::skip(obj);
				if (pinned.has(obj, (byte *)end - fmt::headerSize)) {
					h->setFlag(Header::fMarked);

					result = fmt::Scan<Scanner>::objects(source, obj, end);
					if (result != typename Scanner::Result())
						return result;
				} else {
					h->clearFlag(Header::fMarked);
				}
			}
			return result;
		}

		template <class Scanner>
		typename Scanner::Result Nonmoving::Chunk::scanSweep(typename Scanner::Source &source) {
			typename Scanner::Result result = typename Scanner::Result();
			for (size_t i = 0; i < size; i += header(i)->size() + sizeof(Header)) {
				Header *h = header(i);
				if (h->hasFlag(Header::fFree | Header::fFinalize))
					continue;

				if (h->hasFlag(Header::fMarked)) {
					void *obj = fmt::toClient(h->object());
					result = fmt::Scan<Scanner>::objects(source, obj, fmt::skip(obj));
					if (result != typename Scanner::Result())
						return result;
				} else {
					free(h->object());
				}
			}
			return result;
		}


		/**
		 * Amend another scanner with the ability to detect and correctly handle nonmoving objects.
		 *
		 * This implementation assumes that the regular scanner has a member 'srcGen' that contains
		 * the generation that is interesting to it, so that calling 'fix1' is not necessary. Also
		 * uses the member 'arena' inside the wrapped object to access the arena instance. Also
		 * assumes that there is no difference between 'fix' and 'fixHeader'.
		 */
		template <class Wrap, class Predicate = fmt::ScanAll, bool mark = true>
		class ScanNonmoving {
		public:
			typedef typename Wrap::Result Result;
			typedef typename Wrap::Source Source;

			// Wrapped scanner.
			Wrap wrap;

			// Create.
			ScanNonmoving(Source &source) : wrap(source) {}

			// Check if the pointer is interesting to us.
			inline bool fix1(void *ptr) {
				byte gen = wrap.arena.memGeneration(ptr);
				isNonmoving = gen == nonmovingIdentifier;
				return isNonmoving | (gen == wrap.srcGen);
			}

			// Perform the scanning.
			inline Result fix2(void **ptr) {
				// Is it ours?
				if (isNonmoving)
					return scanNonmoving(ptr);

				return wrap.fix2(ptr);
			}

			SCAN_FIX_HEADER
		private:
			// Was the last object examined with 'fix1' a nonmoving object? We remember this so that
			// we don't have to check again.
			mutable bool isNonmoving;

			// For convenience.
			typedef Nonmoving::Header Header;

			// Scan a pointer to a nonmoving object. If we see one of these, we mark the object as
			// scanned and immediately scan the object recursively. This is fine, since we assume
			// there are few nonmoving objects, which means that the recursion won't be very deep.
			inline Result scanNonmoving(void **ptr) {
				Header *header = Header::fromClient(*ptr);

				// Mark it as reachable.
				if (mark)
					header->setFlag(Header::fMarked);

				// Don't traverse the object in a cycle!
				if (header->hasFlag(Header::fTraversing))
					return Result();

				// Remember we're traversing it.
				header->setFlag(Header::fTraversing);

				typedef RefScanner<ScanNonmoving<Wrap, Predicate, mark>> ScanType;
				Result result = fmt::Scan<ScanType>::objectsIf(Predicate(), *this, *ptr, fmt::skip(*ptr));

				// Now, we're done. Make sure we can traverse it next time as well!
				header->clearFlag(Header::fTraversing);

				return result;
			}
		};

	}
}

#endif
