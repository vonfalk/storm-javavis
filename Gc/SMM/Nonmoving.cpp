#include "stdafx.h"
#include "Nonmoving.h"
#include "Util.h"

#if STORM_GC == STORM_GC_SMM

#include "ArenaTicket.h"

namespace storm {
	namespace smm {

		/**
		 * Nonmoving.
		 */

		Nonmoving::Nonmoving(Arena &arena) : arena(arena), lastChunk(0), memMin(0), memMax(1), toFinalize(null) {}

		Nonmoving::~Nonmoving() {
			for (size_t i = 0; i < chunks.size(); i++)
				arena.freeChunk(chunks[i]->chunk());
		}

		void *Nonmoving::alloc(LockTicket &ticket, const GcType *type) {
			bool finalizer = type->finalizer != null;
			size_t size = fmt::sizeObj(type);

			// If the object has a finalizer, we might want to allocate a bit of extra memory at the
			// end, so that we can make a list of objects in need of finalization!
			void *mem = allocMem(finalizer ? (size + sizeof(void *)) : size);
			void *result = fmt::initObj(mem, type, size);

			if (finalizer)
				fmt::setHasFinalizer(result);
			return result;
		}

		void *Nonmoving::allocMem(size_t size) {
			// Disallow large allocations. We won't do well with them!
			if (size > vmAllocMinSize / 2) {
				dbg_assert(false, L"Trying to allocate a too large object as a non-moving object.");
				return null;
			}

			if (lastChunk >= chunks.size())
				lastChunk = 0;

			for (size_t i = 0; i < chunks.size(); i++) {
				Chunk *chunk = chunks[lastChunk];
				if (void *alloc = chunk->alloc(size))
					return alloc;

				if (++lastChunk >= chunks.size())
					lastChunk = 0;
			}

			size_t newChunk = allocChunk();
			if (newChunk >= chunks.size())
				return null;

			lastChunk = newChunk;
			if (void *alloc = chunks[newChunk]->alloc(size))
				return alloc;

			return null;
		}

		size_t Nonmoving::allocChunk() {
			smm::Chunk alloc = arena.allocChunk(vmAllocMinSize, nonmovingIdentifier);
			if (alloc.empty())
				return chunks.size();

			size_t pos = insertSorted(chunks, Chunk::create(alloc), PtrCompare());
			lastChunk = pos;

			// Update address range.
			{
				size_t low = size_t(alloc.at);
				size_t high = size_t(alloc.end());

				if (chunks.size() == 1) {
					memMin = low;
					memMax = high;
				} else {
					memMin = min(memMin, low);
					memMax = max(memMax, high);
				}
			}

			// TODO: Update range of objects, or other metadata?

			return pos;
		}

		void Nonmoving::free(LockTicket &, void *mem) {
			ChunkList::iterator pos = std::lower_bound(chunks.begin(), chunks.end(), mem, PtrCompare());
			if (pos != chunks.end()) {
				(*pos)->free(this, fmt::fromClient(mem));
			} else {
				dbg_assert(false, L"Trying to 'free' nonmoving memory from a different pool!");
			}

			// TODO: We would like to reclaim memory at some point! Remember to update 'memMin' and 'memMax'!
		}

		void Nonmoving::runFinalizers() {
			Header *first;

			// Grab the list of finalizers to execute.
			do {
				first = atomicRead(toFinalize);
				if (!first)
					return;
			} while (atomicCAS(toFinalize, first, null) != first);


			for (Header *at = first; at; at = at->next()) {
				fmt::Obj *obj = at->object();
				if (first->hasFlag(Header::fFinalize) && fmt::objHasFinalizer(obj))
					fmt::objFinalize(obj);

				// Make sure it doesn't go to the finalization queue again!
				fmt::objClearHasFinalizer(obj);
			}

			// Free them all!
			arena.lock(*this, &Nonmoving::freeChain, first);
		}

		void Nonmoving::freeChain(LockTicket &ticket, Header *first) {
			while (first) {
				Header *next = first->next();

				free(ticket, toClient(first->object()));

				first = next;
			}
		}

		void Nonmoving::runAllFinalizers() {
			for (size_t i = 0; i < chunks.size(); i++)
				chunks[i]->runFinalizers();
		}

		struct Sweeper {
			Nonmoving &owner;
			ArenaTicket &ticket;

			Sweeper(Nonmoving &owner, ArenaTicket &ticket) : owner(owner), ticket(ticket) {}

			void operator ()(void *obj) const {
				Nonmoving::Header *header = Nonmoving::Header::fromClient(obj);
				if (!header->hasFlag(Nonmoving::Header::fMarked)) {
					// Note: This is fine, since it will not break any headers.
					owner.free(ticket, obj);
				}
			}
		};

		void Nonmoving::sweep(ArenaTicket &ticket) {
			// Just call 'free' on all allocations that should be freed!
			traverse(Sweeper(*this, ticket));
		}

		void Nonmoving::fillSummary(MemorySummary &summary) const {
			for (size_t i = 0; i < chunks.size(); i++)
				chunks[i]->fillSummary(summary);
		}

		void Nonmoving::dbg_verify() {
			for (size_t i = 0; i < chunks.size(); i++)
				chunks[i]->dbg_verify();
		}

		void Nonmoving::dbg_dump() {
			PLN(L"Nonmoving allocations, " << chunks.size() << L" chunks:");
			for (size_t i = 0; i < chunks.size(); i++) {
				PNN(i << L": ");
				chunks[i]->dbg_dump();
			}
		}


		/**
		 * Chunk.
		 */

		Nonmoving::Chunk::Chunk(size_t size) : size(size), lastAlloc(0) {
			Header *first = header(0);
			first->makeFree(size - sizeof(Header));
		}

		fmt::Obj *Nonmoving::Chunk::alloc(size_t size) {
			if (fmt::Obj *r = allocRange(size, lastAlloc, this->size))
				return r;
			if (fmt::Obj *r = allocRange(size, 0, lastAlloc))
				return r;

			return null;
		}

		fmt::Obj *Nonmoving::Chunk::allocRange(size_t size, size_t from, size_t to) {
			size_t at = from;
			while (at < to) {
				Header *o = header(at);

				if (o->hasFlag(Header::fFree)) {
					// Since we might not have done so previously, merge with future free blocks!
					mergeFree(o);

					size_t free = o->size();

					if (free >= size) {
						// Split it?
						if (free > size + sizeof(Header) * 3) { // Note: the '3' is a bit arbitrary.
							Header *newObj = header(at + size + sizeof(Header));
							newObj->makeFree(free - size - sizeof(Header));
							o->size(size);
						}

						lastAlloc = at;
						o->clearFlag(Header::fFree);
						return o->object();
					}
				}

				at += o->size() + sizeof(Header);
			}

			return null;
		}

		bool Nonmoving::Chunk::free(Nonmoving *owner, fmt::Obj *obj) {
			Header *h = Header::fromObject(obj);

			if (fmt::objHasFinalizer(h->object())) {
				// If the object has a finalizer, remember it and free it when we have executed finalizers.
				Header *next;
				do {
					next = atomicRead(owner->toFinalize);
					h->next(next);
					h->setFlag(Header::fFinalize);
				} while (atomicCAS(owner->toFinalize, next, h) != next);

				return false;
			} else {
				// Nothing more needs to be done. We can just free it now.
				h->setFlag(Header::fFree);

				// Merge any free blocks that follow us!
				mergeFree(h);

				return true;
			}
		}

		void Nonmoving::Chunk::mergeFree(Header *first) {
			size_t at = offset(first);
			do {
				// Skip ahead to the next one.
				at += header(at)->size() + sizeof(Header);
				// Until the end of the chunk, or until we find a non-free chunk.
			} while (at < size && header(at)->hasFlag(Header::fFree));

			first->makeFree(at - offset(first) - sizeof(Header));
		}

		void Nonmoving::Chunk::runFinalizers() {
			for (size_t at = 0; at < size; at += header(at)->size() + sizeof(Header)) {
				Header *h = header(at);
				if (h->hasFlag(Header::fFree))
					continue;

				if (fmt::objHasFinalizer(h->object())) {
					fmt::objFinalize(h->object());
				}
			}
		}

		void Nonmoving::Chunk::fillSummary(MemorySummary &summary) const {
			summary.allocated += size;
			summary.bookkeeping += sizeof(Chunk);

			for (size_t at = 0; at < size; at += header(at)->size() + sizeof(Header)) {
				summary.bookkeeping += sizeof(Header);

				const Header *h = header(at);
				if (h->hasFlag(Header::fFree))
					summary.free += h->size();
				else
					summary.objects += h->size();
			}
		}

		void Nonmoving::Chunk::dbg_verify() {
			bool lastFound = false;
			size_t at = 0;
			while (at < size) {
				Header *h = header(at);
				assert(h->size() > 0, L"Blocks with size zero should not exist!");

				if (!h->hasFlag(Header::fFree))
					assert(h->size() >= fmt::objSize(h->object()), L"Not enough space is allocated for an object!");

				lastFound |= lastAlloc == at;

				at += h->size() + sizeof(Header);
			}

			assert(at == size, L"A chunk is overfilled!");
			assert(lastFound, L"lastAlloc does not refer to a valid object!");
		}

		void Nonmoving::Chunk::dbg_dump() {
			PLN(L"Chunk at " << (void *)this << L", " << size << L" bytes:");
			for (size_t at = 0; at < size; at += header(at)->size() + sizeof(Header)) {
				if (at == lastAlloc)
					PNN(L"-> ");
				else
					PNN(L"   ");

				Header *h = header(at);
				PNN((void *)h << L", " << h->size() << L" bytes, ");
				if (h->hasFlag(Header::fFree)) {
					PLN(L"free");
				} else {
					PLN(L"allocated");
				}
			}
		}

	}
}

#endif
