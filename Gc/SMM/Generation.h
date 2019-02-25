#pragma once

#if STORM_GC == STORM_GC_SMM

#include "Utils/Lock.h"
#include "Block.h"
#include "InlineSet.h"
#include "ArenaEntry.h"

namespace storm {
	namespace smm {

		class Arena;

		/**
		 * A generation is a set of blocks that belong together, and that will be collected
		 * together. A generation additionaly contains information on the typical size of the
		 * contained blocks, and other useful information.
		 */
		class Generation {
		public:
			// Create a generation, and provide an approximate size of the generation.
			Generation(Arena &owner, size_t size);

			// Destroy.
			~Generation();

			// Lock for this generation.
			util::Lock lock;

			// The next generation in the chain. May be null.
			Generation *next;

			// The size of this generation. We strive to keep below this size, but it may
			// occasionally be broken.
			size_t totalSize;

			// The default size of newly allocated blocks in this generation.
			size_t blockSize;

			// Allocate a new block in this generation. When the block is full, it should be
			// finished by calling 'done'. The size of the returned block is at least 'blockSize'.
			Block *alloc();

			// Notify the generation that a block is full and will no longer be used by an
			// allocator.
			void done(Block *block);

			// Access a shared block that is considered to be at the "end of the generation", where
			// anyone may add objects to this generation. When using this block, the lock needs to
			// be held while the filling is in progress. The parameter indicates how much free
			// memory is needed in the returned block.
			Block *fillBlock(size_t freeBytes);

			// Get an AddrSet initialized to a range suitable for expressing all addresses in this generation.
			template <class AddrSet>
			AddrSet addrSet() const {
				InlineSet<Block>::iterator i = blocks.begin();
				InlineSet<Block>::iterator end = blocks.end();
				if (i == end)
					return AddrSet(0, 1);

				size_t low = size_t((*i)->mem(0));
				size_t high = size_t((*i)->mem((*i)->committed));
				for (++i; i != end; ++i) {
					low = min(low, size_t((*i)->mem(0)));
					high = max(high, size_t((*i)->mem((*i)->committed)));
				}

				return AddrSet(low, high);
			}

			// Get a filled AddrSet indicating all blocks here.
			template <class AddrSet>
			AddrSet filledAddrSet() const {
				AddrSet s = addrSet<AddrSet>();
				for (InlineSet<Block>::iterator i = blocks.begin(), end = blocks.end(); i != end; ++i)
					s.add((*i)->mem(0), (*i)->mem((*i)->committed));
				return s;
			}

			// Collect garbage in this generation (API will likely change).
			void collect(ArenaEntry &entry);

			// Check if this generation *may* contain pointers to the provided block. May return
			// false positives, but never false negatives.
			// TODO: Make sure to re-scan any blocks that contain invalidated or empty summaries the
			// next time they're scanned by someone.
			// TODO: We could implement this a bit more efficiently by keeping a summary that is the
			// union of all summaries here, so that we can do an early-out.
			bool mayReferTo(Block *block) const {
				for (InlineSet<Block>::iterator i = blocks.begin(), end = blocks.end(); i != end; ++i) {
					if ((*i)->mayReferTo(block))
						return true;
				}

				return false;
			}

			// Scan all blocks in this generation using the specified scanner.
			template <class Scanner>
			typename Scanner::Result scan(typename Scanner::Source &source) const {
				typename Scanner::Result r;

				for (InlineSet<Block>::iterator i = blocks.begin(), end = blocks.end(); i != end; ++i) {
					Block *b = *i;
					r = b->scan<Scanner>(source);
					if (r != typename Scanner::Result())
						return r;
				}

				return r;
			}

			// Apply a function to all objects in this generation.
			template <class Fn>
			void walk(Fn &fn) {
				for (InlineSet<Block>::iterator i = blocks.begin(), end = blocks.end(); i != end; ++i) {
					Block *b = *i;
					b->walk(fn);
				}
			}

			/**
			 * Predicate that tells if a particular address is inside the generation or not.
			 */
			struct Contains {
				Contains(AddrSummary overview, const Generation *gen) : overview(overview), gen(gen) {}

				AddrSummary overview;
				const Generation *gen;

				inline bool operator() (void *ptr) const {
					size_t p = size_t(ptr);
					if (overview.has(ptr)) {
						// TODO: We might want to do something faster... If we know 'AddrSummary' is precise enough
						// then we could rely entirely on that.
						for (InlineSet<Block>::iterator i = gen->blocks.begin(), end = gen->blocks.end(); i != end; ++i) {
							Block *b = *i;
							if (p >= size_t(b->mem(0)) && p < size_t(b->mem(b->committed)))
								return true;
						}
					}

					return false;
				}
			};

			// Create a predicate that checks if a particular address is contained within this generation.
			inline Contains containsPredicate() const {
				return Contains(filledAddrSet<AddrSummary>(), this);
			}

		private:
			// No copy.
			Generation(const Generation &o);
			Generation &operator =(const Generation &o);

			// Owning arena.
			Arena &owner;

			// All blocks in this generation, including 'sharedBlock'.
			InlineSet<Block> blocks;

			// The last block, currently being filled. May be null.
			Block *sharedBlock;

			// Allocate blocks with a specific size. For internal use.
			Block *allocBlock(size_t size);
		};

	}
}

#endif
