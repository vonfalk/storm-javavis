#include "stdafx.h"
#include "VMAlloc.h"

#if STORM_GC == STORM_GC_SMM

#include "Gc/Gc.h"

namespace storm {
	namespace smm {

		wostream &operator <<(wostream &to, const Chunk &c) {
			return to << L"Chunk: " << c.at << L"+" << c.size;
		}

		VMAlloc::VMAlloc(VM *vm, size_t initSize) :
			vm(vm), pageSize(vm->pageSize),
			minAddr(0), maxAddr(1),
			info(null), lastAlloc(0) {

			size_t granularity = max(vmAllocMinSize, vm->allocGranularity);

			// Try to allocate the initial block!
			initSize = roundUp(initSize, granularity);
			void *mem = vm->reserve(null, initSize);
			while (!mem) {
				initSize /= 2;
				if (initSize < granularity)
					throw GcError(L"Unable to allocate memory for the initial arena!");
				mem = vm->reserve(null, initSize);
			}
			addReserved(Chunk(mem, initSize));
		}

		VMAlloc::~VMAlloc() {
			for (size_t i = 0; i < reserved.size(); i++)
				vm->free(reserved[i].at, reserved[i].size);

			delete vm;
		}

		void VMAlloc::updateMinMax(size_t &minAddr, size_t &maxAddr) {
			if (reserved.empty()) {
				minAddr = 0;
				maxAddr = 1;
			}

			minAddr = size_t(reserved[0].at);
			maxAddr = minAddr + reserved[0].size;

			for (size_t i = 1; i < reserved.size(); i++) {
				size_t base = size_t(reserved[i].at);
				minAddr = min(minAddr, base);
				maxAddr = max(maxAddr, base + reserved[i].size);
			}
		}

		void VMAlloc::addReserved(Chunk chunk) {
			reserved.push_back(chunk);

			size_t totalMin, totalMax;
			updateMinMax(totalMin, totalMax);

			size_t count = infoCount(totalMin, totalMax);
			byte *totalInfo = null;
			if (info) {
				// Note: This is a bit tricky. Since allocations may not be aligned to our
				// granularity by the underlying system's memory manager, we might have to adjust
				// 'min' and 'max' a bit so that the alignment of the old data structure remains
				// correct. (Example: old allocation was at 0x20000, new allocation is at 0x11000
				// with an alignment of 0x10000 will require poking 'min' to 0x10000, otherwise
				// blocks in the old allocation are shifted by 0x1000). This will sadly waste some
				// of the virtual address space, but that is fine as we don't expect to have many
				// disjoint blocks of memory to manage.
				assert(false, L"TODO: Update the already existing memory info!");
			} else {
				// New allocation, just put it in the beginning of the first block. This will always
				// work since the info table will always be smaller than the one single allocation.
				vm->commit(chunk.at, roundUp(count, vmAllocMinSize));
				totalInfo = (byte *)chunk.at;
				memset(totalInfo, INFO_FREE, count);
			}

			minAddr = totalMin;
			maxAddr = totalMax;
			info = totalInfo;

			// Mark the new information struct as 'allocated' so that we don't overwrite it later!
			for (size_t at = infoOffset(info), to = infoOffset(info + count); at < to; at++) {
				info[at] = INFO_USED;
			}
		}

		static inline size_t findRange(byte *in, size_t from, size_t to, size_t size) {
			size_t start = from;
			for (size_t i = from; i < to; i++) {
				if (in[i] & 0x01) {
					start = i + 1;
				} else if (i - start + 1 >= size) {
					return start;
				}
			}

			return to;
		}


		Chunk VMAlloc::alloc(size_t size, byte identifier) {
			size_t pieces = (size + vmAllocMinSize - 1) / vmAllocMinSize;

			size_t wrap = infoCount();
			size_t start = findRange(info, lastAlloc, wrap, pieces);
			if (start >= wrap) {
				start = findRange(info, 0, min(wrap, lastAlloc + pieces), pieces);

				// Out of memory!
				if (start > lastAlloc) {
					TODO(L"We should try to reserve more memory here!");
					return Chunk();
				}
			}

			lastAlloc = start + pieces;

			// Mark the memory as in use.
			memset(info + start, INFO_USED | ((identifier & 0x3F) << 2), pieces);

			Chunk mem(infoPtr(start), pieces*vmAllocMinSize);
			vm->commit(mem.at, mem.size);
			return mem;
		}

		void VMAlloc::free(Chunk chunk) {
			// Mark as free.
			memset(info + infoOffset(chunk.at), 0, chunk.size / vmAllocMinSize);

			// Decommit from OS.
			vm->decommit(chunk.at, chunk.size);
		}

		void VMAlloc::checkWrites(void **buffer) {
			vm->notifyWrites(this, buffer);
		}

		void VMAlloc::markBlockWrites(void **addr, size_t count) {
			for (size_t i = 0; i < count; i++) {
				info[infoOffset(addr[i])] |= INFO_WRITTEN;
			}
		}

	}
}

#endif
