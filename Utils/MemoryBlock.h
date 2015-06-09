#pragma once

#include <iostream>

namespace memory {

	class Block {
	public:
		Block(nat desiredSize);
		~Block();

		inline nat largestFree() const { return largestFreeSpace; };

		void *allocate(nat size);
		void free(void *ptr);

		bool empty() const;

		byte *backup() const;
		bool verify(byte *b) const;

		static nat getResolution();
		static Block *owningBlock(void *allocated); //Get the Block which owns the allocation.

		class Compare {
		public:
			bool operator ()(const Block *a, const Block *b) const;
		};

		friend std::wostream &operator <<(std::wostream &to, const Block &block);
	private:
		struct Descriptor {
			unsigned allocated : 1;
			unsigned size : 31;
			void *ownerOrNext;

			void initialize(bool allocated, nat size, void *owner);
		};

		byte *data;
		byte *firstFree;
		nat allocatedChunks;
		nat size;
		nat largestFreeSpace;

		void initialize();
		void updateLargestFree();

		void print(std::wostream &to) const;
		void print(std::wostream &to, byte *block) const;

		bool valid() const;

		static nat resolution;
		static const nat overhead = sizeof(Descriptor); //The overhead for each allocation.
		static const nat smallestSize = 16; //Smallest free chunk.
		static const nat alignment = 4; //Must be a power of two.
		static inline Descriptor &descriptor(void *at) { return *((Descriptor *)at); };
	};
}
