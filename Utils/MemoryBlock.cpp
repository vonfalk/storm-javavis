#include "stdafx.h"

#include "MemoryBlock.h"

namespace memory {

	Block::Block(nat desiredSize) : allocatedChunks(0) {
		desiredSize += overhead;

		nat resolution = getResolution();
		nat above = desiredSize % resolution;
		if (above == 0) {
			size = desiredSize;
		} else {
			size = desiredSize + (resolution - above);
		}
		data = (byte *)VirtualAlloc(null, size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

		initialize();
	}

	Block::~Block() {
		VirtualFree(data, 0, MEM_RELEASE);
	}

	void Block::initialize() {
		descriptor(data).initialize(false, size - overhead, null);
		firstFree = data;

		updateLargestFree();
	}

	void *Block::allocate(nat size) {
		nat remainder = size & (alignment - 1);
		if (remainder) size += alignment - remainder;

		if (size > largestFreeSpace) return null;

		void **beforeBest = (void **)&firstFree;
		byte *bestMatch = null;

		void **prev = (void **)&firstFree;
		for (byte *at = firstFree; at; prev = &descriptor(at).ownerOrNext, at = (byte *)descriptor(at).ownerOrNext) {
			if (bestMatch) {
				nat dSize = descriptor(at).size;
				if (dSize == size) {
					beforeBest = prev;
					bestMatch = at;
					break;
				} else if (dSize > size) {
					if (descriptor(at).size < descriptor(bestMatch).size) {
						bestMatch = at;
						beforeBest = prev;
					}
				}
			} else if (descriptor(at).size >= size) {
				beforeBest = prev;
				bestMatch = at;
			}
		}

		if (bestMatch == null) return null;

		Descriptor &best = descriptor(bestMatch);
		if (best.size > size + overhead + smallestSize) {
			descriptor(bestMatch + size + overhead).initialize(false, best.size - size - overhead, best.ownerOrNext);
			best.size = size;
			*beforeBest = bestMatch + size + overhead;
		} else {
			*beforeBest = (byte *)best.ownerOrNext;
		}
		best.allocated = 1;
		best.ownerOrNext = this;
		updateLargestFree();
		allocatedChunks++;

		assert(valid());
		assert(owningBlock(bestMatch + overhead) == this);
		return bestMatch + overhead;
	}

	void Block::free(void *p) {
		byte *ptr = (byte *)p;
		ptr -= overhead;
		assert(ptr >= data && ptr <= data + size);
		assert(descriptor(ptr).ownerOrNext == this);
		
		byte *last = null;
		void **lastNext = (void **)&firstFree;
		for (byte *at = data; at < data + size; at += descriptor(at).size + overhead) {
			Descriptor &now = descriptor(at);

			if (at == ptr) {
				assert(now.allocated == 1); //Else double free!
				assert(allocatedChunks > 0);
				allocatedChunks--;
				now.allocated = 0;
				now.ownerOrNext = *lastNext;
				*lastNext = at;

#ifdef DEBUG
				// Clear any free'd memory in debug mode, to avoid strange bugs.
				memset(at + sizeof(Descriptor), 0, now.size);
#endif
			}

			if (last != null) {
				if (descriptor(last).allocated == 0 && now.allocated == 0) {
					descriptor(last).size += now.size + overhead;
					descriptor(last).ownerOrNext = now.ownerOrNext;
					at = last;
				}
			}

			if (descriptor(at).allocated == 0) {
				largestFreeSpace = max(largestFreeSpace, descriptor(at).size);
				lastNext = &now.ownerOrNext;
			}
			last = at;
		}

		assert(valid());
	}

	bool Block::empty() const {
		return allocatedChunks == 0;
	}

	bool Block::valid() const {
		nat size = 0;
		nat freeSize = 0;
		for (byte *at = data; at < data + this->size; at += descriptor(at).size + overhead) {
			Descriptor &d = descriptor(at);
			size += d.size + overhead;
			if (d.allocated == 1) {
				if (d.ownerOrNext != this) {
					return false;
				}
			} else {
				freeSize += d.size + overhead;
			}
		}
		assert(size == this->size);

		nat freeListSize = 0;
		for (byte *at = firstFree; at; at = (byte *)descriptor(at).ownerOrNext) {
			assert(descriptor(at).allocated == 0);
			freeListSize += descriptor(at).size + overhead;
		}
		if (freeSize != freeListSize) std::wcout << *this;
		assert(freeSize == freeListSize);
		return true;
	}

	void Block::updateLargestFree() {
		largestFreeSpace = 0;

		//for (byte *at = data; at < data + size; at += descriptor(at).size + overhead) {
		for (byte *at = firstFree; at; at = (byte *)descriptor(at).ownerOrNext) {
			assert(descriptor(at).allocated == 0);
			largestFreeSpace = max(largestFreeSpace, descriptor(at).size);
		}
	}

	void Block::print(std::wostream &to) const {
		using std::endl;

		to << L"Block @" << (void *)data << L"(" << (void *)this << L")" << endl;
		to << L" Largest free: " << largestFreeSpace << endl;
		to << L" Allocated chunks: " << allocatedChunks << endl;

		for (byte *at = data; at < data + size; at += descriptor(at).size + overhead) {
			print(to, at);
		}

		to << L" Free:" << endl;

		for (byte *at = firstFree; at; at = (byte *)descriptor(at).ownerOrNext) {
			print(to, at);
		}
	}

	void Block::print(std::wostream &to, byte *block) const {
		using std::endl;

		to << L"  @" << (void *)(block + overhead) << L" (" << (void *)(block) << L")";
		to << L", " << descriptor(block).size << L" b:";
		to << (descriptor(block).allocated == 1 ? L"allocated" : L"free") << endl;
	}

	byte *Block::backup() const {
		byte *backupData = new byte[size];

		memcpy_s(backupData, size, data, size);

		return backupData;
	}

	bool Block::verify(byte *b) const {
		for (byte *at = data; at < data + size; at += descriptor(at).size + overhead) {
			if (descriptor(at).allocated == 1) {
				int offset = int(at - data);
				Descriptor &current = descriptor(at);
				Descriptor &old = descriptor(&b[offset]);
				if (old.allocated == 1) {
					for (nat i = 0; i < descriptor(at).size + overhead; i++) {
						byte *addr = &data[offset + i];
						if (data[offset + i] != b[offset + i]) {
							delete b;
							return false;
						}
					}
				}
			}
		}

		delete b;
		return true;
	}

	nat Block::resolution = 0;

	nat Block::getResolution() {
		if (resolution == 0) {
			SYSTEM_INFO sysInfo;
			GetSystemInfo(&sysInfo);
			resolution = sysInfo.dwPageSize;
		}
		return resolution;
	}

	Block *Block::owningBlock(void *allocated) {
		byte *at = ((byte *)allocated) - overhead;
		Descriptor &d = descriptor(at);
		if (d.allocated == 0) return null;
		return (Block *)d.ownerOrNext;
	}

	std::wostream &operator <<(std::wostream &to, const Block &from) {
		from.print(to);
		return to;
	}

	//////////////////////////////////////////////////////////////////////////
	// Descriptor
	//////////////////////////////////////////////////////////////////////////

	void Block::Descriptor::initialize(bool allocated, nat size, void *owner) {
		this->allocated = (allocated ? 1 : 0);
		this->size = size;
		this->ownerOrNext = owner;
	}

	//////////////////////////////////////////////////////////////////////////
	// Compare
	//////////////////////////////////////////////////////////////////////////

	bool Block::Compare::operator ()(const Block *a, const Block *b) const {
		if (a->largestFree() == b->largestFree()) return a->data < b->data;
		return a->largestFree() < b->largestFree();
	}
}
