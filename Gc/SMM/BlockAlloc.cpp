#include "stdafx.h"
#include "BlockAlloc.h"

#if STORM_GC == STORM_GC_SMM

#include "Gc/Gc.h"

namespace storm {
	namespace smm {

		/**
		 * Contents of the 'info' member describing each chunk.
		 *
		 * Each byte represents 'blockMinSize' bytes of memory, starting from 'minAddr'. Each byte
		 * indicates whether or not that particular block is used, and other data that can be
		 * retrieved from the public API (e.g. which generation the memory belongs to and other
		 * information that is used to quickly determine if a particular pointer should be collected
		 * or not).
		 *
		 * The content of each byte is as follows:
		 * Bit 0: In use (1) or free (0).
		 * Bit 1: Contents altered since last check?
		 * Bit 2-7: User data.
		 */

		// Convenient constants.
		static const byte INFO_FREE = 0x00;
		static const byte INFO_USED = 0x01;

		static inline bool inUse(byte b) { return b & 0x01; }
		static inline bool changed(byte b) { return (b >> 1) & 0x01; }
		static inline byte data(byte b) { return b >> 2; }


		BlockAlloc::BlockAlloc(VM *vm, size_t initSize) : vm(vm), pageSize(vm->pageSize), minAddr(0), maxAddr(1), info(null) {
			size_t granularity = max(blockMinSize, vm->allocGranularity);

			// Try to allocate the initial block!
			initSize = roundUp(initSize, granularity);
			void *mem = vm->reserve(null, initSize);
			while (!mem) {
				initSize /= 2;
				if (initSize < granularity)
					throw GcError(L"Unable to allocate memory for the initial arena!");
				mem = vm->reserve(null, initSize);
			}
			addChunk(mem, initSize);
		}

		BlockAlloc::~BlockAlloc() {
			for (size_t i = 0; i < chunks.size(); i++)
				vm->free(chunks[i].at, chunks[i].pages * pageSize);

			delete vm;
		}

		void BlockAlloc::updateMinMax(size_t &minAddr, size_t &maxAddr) {
			if (chunks.empty()) {
				minAddr = 0;
				maxAddr = 1;
			}

			minAddr = size_t(chunks[0].at);
			maxAddr = minAddr + chunks[0].pages * pageSize;

			for (size_t i = 1; i < chunks.size(); i++) {
				size_t base = size_t(chunks[i].at);
				minAddr = min(minAddr, base);
				maxAddr = max(maxAddr, base + chunks[i].pages * pageSize);
			}
		}

		void BlockAlloc::addChunk(void *mem, size_t size) {
			chunks.push_back(Chunk(mem, size / pageSize));

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
				// New allocation, just put it in the beginning of the first block.
				vm->commit(mem, roundUp(count, blockMinSize));
				totalInfo = (byte *)mem;
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

	}
}

#endif
