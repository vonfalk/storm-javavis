#pragma once

#if STORM_GC == STORM_GC_SMM

#include "GenSet.h"
#include "VMAlloc.h"
#include "Format.h"

namespace storm {
	namespace smm {

		class VMAlloc;

		/**
		 * A block of memory allocated in some generation with some associated metadata used by the
		 * generation for bookkeeping. The Block class represents the header in the beginning of an
		 * allocation, and the remainder of the allocation is usable to store objects.
		 *
		 * We assume memory inside the blocks are allocated from low addresses to high addresses
		 * simply by bumping a pointer, and the Block keeps track of this. Blocks are never moved,
		 * even though objects inside a block may move inside or between blocks. However,
		 * generations may chose to re-create a block around previously allocated data in cases
		 * where some allocations may not be moved. This is, however, considered to be a new block
		 * without any connection to the previous block.
		 */
		class Block {
		public:
			// Create.
			Block(size_t size) : size(size), commit(0), reserveNext(0), flags(0) {}

			// Current size (excluding the block itself).
			const size_t size;

			// Various flags for this block.
			enum Flags {
				// This block is in use by an allocator.
				fUsed = 0x00000001,

				// This block may contain at least one object that needs finalization.
				fFinalizers = 0x00000002,

				// This block should not be scanned the next time we attempt to.
				fSkipScan = 0x00000004,
			};

			// Modify flags.
			inline bool hasFlag(Flags flag) {
				return (atomicRead(flags) & flag) != 0;
			}
			inline void setFlag(Flags flag) {
				atomicOr(flags, flag);
			}
			inline void clearFlag(Flags flag) {
				atomicAnd(flags, ~flag);
			}

			// Amount of memory committed (excluding the block itself).
			size_t committed() const { return atomicRead(commit); }
			void committed(size_t val) { atomicWrite(commit, val); }

			// Perform a CAS on 'committed'. Returns 'true' if successfully updated.
			bool committedCAS(size_t check, size_t replace) {
				return atomicCAS(commit, check, replace) == check;
			}

			// Amount of memory reserved. 'reserved >= committed'. Memory that is reserved but
			// not committed is being initialized, and can not be assumed to contain usable
			// data. If 'next' is used, this value is set to 'size'.
			size_t reserved() const {
				size_t v = atomicRead(reserveNext);
				if (v & 0x1)
					return size;
				else
					return v;
			}
			void reserved(size_t val) {
				dbg_assert((val & 0x1) == 0, L"LSB must be clear!");
				atomicWrite(reserveNext, val);
			}

			// Pointer to another block. Used when copying blocks to link a filled block to the next
			// block where objects are copied to. Whenever 'next' is used, 'reserved' is set to
			// 'size', since the two fields use the same storage.
			Block *next() const {
				size_t v = atomicRead(reserveNext);
				if (v & 0x1)
					return (Block *)(v & ~size_t(0x1));
				else
					return null;
			}
			void next(Block *val) {
				atomicWrite(reserveNext, size_t(val) | 0x1);
			}

			// Get the number of bytes remaining in this block (ignoring any reserved memory).
			inline size_t remaining() const {
				return size - committed();
			}

			// Get a pointer to a particular byte of the memory in this block.
			inline void *mem(size_t offset) {
				void *ptr = this;
				return (byte *)ptr + sizeof(Block) + offset;
			}

			// Get an AddrSet initialized to the range contained in the block.
			template <class AddrSet>
			AddrSet addrSet() {
				return AddrSet(mem(0), mem(committed()));
			}

			// Add padding to the end of the block if required, so that it is safe to use the 'next' pointer.
			void padReserved() {
				size_t reserved = this->reserved();
				if (reserved < size) {
					fmt::objMakePad((fmt::Obj *)mem(reserved), size - reserved);
					this->reserved(size);
				}
			}

			// Is it possible that this block contains references to a generation with the specified
			// number?
			bool mayReferTo(byte gen) const {
				// TODO: Check stale memory!
				return true;

				return summary.has(gen);
			}
			bool mayReferTo(GenSet gen) const {
				// TODO: Check stale memory!
				return true;

				return summary.has(gen);
			}

			// Traverse all objects in this block. Calling the supplied function on each
			// object. Client pointers are passed to the function.
			template <class Fn>
			void traverse(Fn fn) {
				fmt::Obj *at = (fmt::Obj *)mem(0);
				fmt::Obj *end = (fmt::Obj *)mem(committed());
				while (at != end) {
					fmt::Obj *next = fmt::objSkip(at);

					fn(fmt::toClient(at));

					at = next;
				}
			}

			// Scan all objects in this block using the supplied scanner.
			template <class Scanner>
			typename Scanner::Result scan(typename Scanner::Source &source) {
				// TODO: Perhaps we should update our summary here?
				return fmt::Scan<Scanner>::objects(source, mem(fmt::headerSize), mem(fmt::headerSize + committed()));
			}

			// Scan all objects that fulfill a specified predicate using the supplied scanner.
			template <class Predicate, class Scanner>
			typename Scanner::Result scanIf(const Predicate &predicate, typename Scanner::Source &source) {
				return fmt::Scan<Scanner>::template objectsIf<Predicate>(predicate, source,
																		mem(fmt::headerSize),
																		mem(fmt::headerSize + committed()));
			}

			// Fill memory summary.
			void fillSummary(MemorySummary &summary);

			// Verify the contents of this block.
			void dbg_verify();

			// Output a summary of this block.
			void dbg_dump();

		private:
			// No copying!
			Block(const Block &o);
			Block &operator =(const Block &o);

			// Bytes committed.
			size_t commit;

			// Bytes reserved, or a pointer to another block (when the LSB is set).
			size_t reserveNext;

			// Our flags (updated atomically by the corresponding functions).
			size_t flags;

			// A summary of the references contained in here.
			GenSet summary;

			// Arrange so that we will be notified about writes to the contents of this block. Clears 'fUpdated' flag.
			void watchWrites();
		};

	}
}

#endif
