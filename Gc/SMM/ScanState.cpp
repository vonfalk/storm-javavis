#include "stdafx.h"
#include "ScanState.h"
#include "Nonmoving.h"
#include "ArenaTicket.h"

#if STORM_GC == STORM_GC_SMM

namespace storm {
	namespace smm {

		ScanState::ScanState(ArenaTicket &ticket, const Generation::State &from, Generation *to) :
			sourceGen(from),
			target(to, ticket), weak(to, ticket) {

			// This could cause 'alloc' below to fail!
			assert(from.gen.blockSize <= to->blockSize,
				L"Can not copy objects between generations with decreasing block size!");
		}

		ScanState::~ScanState() {
			// TODO: Make sure that everything is properly scanned? Otherwise, we will lose objects.
		}

		void *ScanState::move(void *client) {
			fmt::Obj *obj = fmt::fromClient(client);
			size_t size = fmt::objSize(obj);

			// PLN(L"Found an object to move: " << obj << L", " << size << L" bytes");

			Queue *to = &target;
			if (fmt::objIsWeak(obj)) {
				to = &weak;

				// We need to scan the object reference of the weak object now. Otherwise, the type
				// information could be lost since we would otherwise treat it as a weak reference.
				// This involves some amount of recursion, but since the GcType objects never
				// contain any weak references, this is fine. We also make sure to only scan the
				// header here.
				typedef OnlyHeader<ScanNonmoving<Move, true>> Scanner;
				fmt::Scan<Scanner>::objects(*this, client, (byte *)client + size);
			}

			Block *tail = to->tail;
			if (!tail || tail->size - tail->reserved() < size) {
				to->newBlock(size);
				tail = to->tail;
			}

			size_t off = tail->reserved();
			fmt::Obj *target = (fmt::Obj *)tail->mem(off);
			memcpy(target, obj, size);
			tail->reserved(off + size);

			// Check if the object is registered for finalization, and update the target block accordingly.
			if (fmt::objHasFinalizer(target))
				tail->setFlag(Block::fFinalizers);

			void *clientTarget = fmt::toClient(target);
			fmt::objMakeFwd(obj, size, clientTarget);

			return clientTarget;
		}

		void ScanState::scanNew() {
			int error = 0;
			typedef ScanNonmoving<NoWeak<Move>, true> Scanner;
			while (target.scanStep<Scanner>(*this, error)) {
				dbg_assert(error == 0, L"TODO: We need to handle allocation errors while scanning!");
			}
		}


		/**
		 * The queue.
		 */

		ScanState::Queue::~Queue() {
			// Call 'done' on all blocks we have allocated earlier.
			while (head) {
				Block *b = head;
				head = head->next();
				target->done(ticket, b);
			}
		}

		void ScanState::Queue::newBlock(size_t minSize) {
			// TODO: We might want to handle cases where 'minSize' is large enough not to fit
			// here. This should not happen if generation sizes are set up appropriately, however.
			Block *n = target->alloc(ticket, minSize);
			if (!n) {
				target->arena.dbg_dump();
				TODO(L"Handle out of memory conditions! We attempted to allocate " << minSize << " bytes.");
				assert(false);
			}

			if (tail) {
				tail->padReserved();
				tail->next(n);
			} else {
				// Empty list. Put the same one at both locations.
				head = n;
			}

			// If it is empty, we don't need to scan it during the cleanup phase. That has already
			// been done by us.
			if (n->committed() == 0)
				n->setFlag(Block::fSkipScan);

			tail = n;
		}

	}
}

#endif
