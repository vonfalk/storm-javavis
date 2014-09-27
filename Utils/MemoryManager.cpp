#include "stdafx.h"

#include "MemoryManager.h"

//Can be played back with memoryDumpTest in LinkerTest
//#define LOG_OUTPUT_TO "memory.log"

#ifdef LOG_OUTPUT_TO
#include <fstream>
#include <iomanip>
#endif

namespace memory {

#ifdef LOG_OUTPUT_TO
	std::ofstream file(LOG_OUTPUT_TO);
#endif

	Manager::Manager() {}

	Manager::~Manager() {
		for (BlockSet::iterator i = blocks.begin(); i != blocks.end(); i++) {
			delete *i;
		}
		blocks.clear();
	}

	void *Manager::allocate(nat bytes) {
#ifdef LOG_OUTPUT_TO
		file << "+ " << bytes << " ";
#endif
		for (BlockSet::iterator i = blocks.begin(); i != blocks.end(); i++) {
			Block *b = *i;
			if (b->largestFree() >= bytes) {
				blocks.erase(b);
				//byte *backup = b->backup();
				void *newMem = b->allocate(bytes);
				//b->verify(backup);
#ifdef LOG_OUTPUT_TO
				file << newMem << std::endl;
#endif
				blocks.insert(b);
				return newMem;
			}
		}

		Block *b = new Block(bytes);
		//byte *backup = b->backup();
		void *newMem = b->allocate(bytes);
		//b->verify(backup);
#ifdef LOG_OUTPUT_TO
		file << newMem << std::endl;
#endif
		blocks.insert(b);
		return newMem;
	}

	void Manager::free(void *memptr) {
#ifdef LOG_OUTPUT_TO
		file << "- " << std::hex << nat(memptr) << std::endl;
#endif
		Block *b = Block::owningBlock(memptr);
		assert(blocks.count(b) == 1);
		if (b == null) return;

		blocks.erase(b);
		//byte *backup = b->backup();
		b->free(memptr);
		//ASSERT(b->verify(backup));
		if (b->empty()) {
			delete b;
		} else {
			blocks.insert(b);
		}
	}

	void Manager::print(std::wostream &to) const {
		to << L"Manager:" << std::endl;
		for (BlockSet::const_iterator i = blocks.begin(); i != blocks.end(); i++) {
			to << *(*i);
		}
		to << std::endl;
	}

	std::wostream &operator <<(std::wostream &to, const Manager &from) {
		from.print(to);
		return to;
	}
}
