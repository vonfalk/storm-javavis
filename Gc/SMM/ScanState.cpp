#include "stdafx.h"
#include "ScanState.h"

#if STORM_GC == STORM_GC_SMM

namespace storm {
	namespace smm {

		ScanState::ScanState(ArenaTicket &ticket, Generation::State &from, Generation *to) :
			ticket(ticket),
			sourceGen(from), targetGen(to),
			targetHead(null), targetTail(null) {

			// This could cause 'alloc' below to fail!
			assert(sourceGen.gen.blockSize <= targetGen->blockSize,
				L"Can not copy objects between generations with decreasing block size!");
		}

		ScanState::~ScanState() {
			// TODO: Make sure that everything is properly scanned? Otherwise, we will lose objects.


			// Call 'done' on all blocks we have allocated earlier.
			while (targetHead) {
				Block *b = targetHead;
				targetHead = targetHead->next();
				targetGen->done(ticket, b);
			}
		}

		void *ScanState::move(void *client) {
			fmt::Obj *obj = fmt::fromClient(client);
			size_t size = fmt::objSize(obj);

			// PLN(L"Found an object to move: " << obj << L", " << size << L" bytes");

			if (!targetTail || targetTail->size - targetTail->reserved() < size)
				newBlock(size);

			size_t off = targetTail->reserved();
			fmt::Obj *target = (fmt::Obj *)targetTail->mem(off);
			memcpy(target, obj, size);
			targetTail->reserved(off + size);

			// Check if the object is registered for finalization, and update the target block accordingly.
			if (fmt::objHasFinalizer(target))
				targetTail->setFlag(Block::fFinalizers);

			void *clientTarget = fmt::toClient(target);
			fmt::objMakeFwd(obj, size, clientTarget);
			return clientTarget;
		}

		void ScanState::newBlock(size_t minSize) {
			// TODO: We might want to handle cases where 'minSize' is large enough not to fit
			// here. This should not happen if generation sizes are set up appropriately, however.
			Block *n = targetGen->alloc(ticket, minSize);
			if (!n) {
				TODO(L"Handle out of memory conditions!");
				assert(false);
			}

			if (targetTail) {
				targetTail->padReserved();
				targetTail->next(n);
			} else {
				// Empty list. Put the same one at both locations.
				targetHead = n;
			}

			targetTail = n;
		}

		void ScanState::scanNew() {
			while (scanStep())
				;
		}

		bool ScanState::scanStep() {
			Block *b = targetHead;
			if (!b)
				return false;

			size_t from = b->committed();
			size_t to = b->reserved();
			if (from >= to)
				return false;

			// TODO: Handle errors!
			fmt::Scan<Move>::objects(*this, b->mem(from + fmt::headerSize), b->mem(to + fmt::headerSize));
			b->committed(to);

			// Are we done scanning this block?
			if (to == b->size) {
				Block *next = b->next();
				if (next) {
					targetHead = next;
				} else {
					targetHead = null;
					targetTail = null;
				}

				targetGen->done(ticket, b);
			}

			return targetHead != null;
		}

	}
}

#endif
