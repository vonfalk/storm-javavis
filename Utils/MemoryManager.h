#pragma once

#include "MemoryBlock.h"

#include <set>
#include <iostream>

namespace memory {

	// An implementation of an allocator with similar properties to
	// malloc and free. The difference is that this one allocates from
	// memory pages with the execute bit set. This manager is suitable
	// for allocations with size below the page size, as it will allocate
	// a page at a time unless larger chunks are needed.
	class Manager {
	public:
		Manager();
		~Manager();

		void *allocate(nat bytes);
		void free(void *memptr);

		inline bool empty() const { return blocks.empty(); };

		friend std::wostream &operator <<(std::wostream &to, const Manager &from);
	private:
		typedef set<Block *, Block::Compare> BlockSet;
		BlockSet blocks;

		void print(std::wostream &to) const;
	};

}