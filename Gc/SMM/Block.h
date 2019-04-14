#pragma once

#if STORM_GC == STORM_GC_SMM

#include "InlineSet.h"
#include "AddrSet.h"
#include "VMAlloc.h"
#include "Gc/Format.h"

namespace storm {
	namespace smm {

		class VMAlloc;

		/**
		 * A block of memory allocated by an Arena, with some associated metadata required by the
		 * collector. Blocks are generally allocated as a large chunk, where the Block instance is
		 * in the beginning of the allocation, and the remainder of the allocation is the actual
		 * memory.
		 *
		 * Blocks can be considered to be garbage collected, as they are removed (or recycled) when
		 * all objects inside them have been removed.
		 *
		 * We assume memory inside the blocks are allocated from low addresses to high addressed
		 * simply by bumping a pointer, and the Block keeps track of this. Blocks are never moved,
		 * even though objects inside a block may move inside or between blocks.
		 */
		class Block : public SetMember<Block> {
		public:
			Block(VMAlloc *inside, size_t size)
				: size(size), committed(0), reserved(0), flags(fUpdated), inside(inside), summary(0, 1) {}

			// Current size (excluding the block itself).
			const size_t size;

			// Amount of memory committed (excluding the block itself).
			size_t committed;

			// Amount of memory reserved. 'reserved >= committed'. Memory that is reserved but not
			// committed is being initialized, and can not be assumed to contain usable data.
			size_t reserved;

			// Various flags for this block.
			enum Flags {
				// This block is empty and can be deallocated. Used during GC phases.
				fEmpty = 0x01,

				// This block is swept and can be cleaned without losing data.
				fSwept = 0x02,

				// This block has been written to since the last call to 'watchWrites'.
				fUpdated = 0x04,
			};
			size_t flags;

			// Get the number of bytes remaining in this block (ignoring any reserved memory).
			inline size_t remaining() const {
				return size - committed;
			}

			// Get a pointer to a particular byte of the memory in this block.
			inline void *mem(size_t offset) {
				void *ptr = this;
				return (byte *)ptr + sizeof(Block) + offset;
			}

			// Get an AddrSet initialized to the range contained in the block.
			template <class AddrSet>
			AddrSet addrSet() {
				return AddrSet(mem(0), mem(committed));
			}

			// Is it possible that this block contains a reference to the block indicated in the
			// parameter? May return false positives, but never false negatives.
			bool mayReferTo(Block *block) const {
				if (flags & fUpdated)
					// Empty summary or stale summary.
					return true;

				return summary.has(block->mem(0), block->mem(block->committed));
			}

			// Scan this block using a scanner.
			template <class Scanner>
			typename Scanner::Result scan(typename Scanner::Source &source) {
				// Always reset 'reserved' when we scan. Otherwise, we might miss something
				// important if we're scanning during an allocation.
				reserved = committed;
				summary = inside->addrSet<AddrSummary>();
				typedef WithSummary<AddrSummary, Scanner> S;
				typename Scanner::Result r = fmt::Scan<S>::objects(
					S::Source(summary, source),
					// Note: We need to pass client pointers to the scanning functions.
					mem(0 + fmt::headerSize),
					mem(committed + fmt::headerSize));

				// TODO: We might not always want to trigger the memory watch, it may be expensive.
				watchWrites();

				return r;
			}

			// Iterate through each object and apply the function to them. Obj * pointers are passed
			// to the provided function.
			template <class Fn>
			void walk(Fn &fn) {
				fmt::Obj *end = (fmt::Obj *)mem(committed);
				for (fmt::Obj *at = (fmt::Obj *)mem(0); at != end; at = fmt::objSkip(at)) {
					fn(at);
				}
			}

			// Sweep this block, replacing any unmarked objects with forwarders to null. Returns
			// 'true' if the block contained any live (marked) objects.
			bool sweep();

			// Clean this block, replacing sequences of forwarding objects and padding with padding
			// and decrease "committed" and "reserved" as much as possible.
			void clean();

		private:
			// No copying!
			Block(const Block &o);
			Block &operator =(const Block &o);

			// A summary of objects we refer to.
			AddrSummary summary;

			// The VMAlloc we're allocated inside.
			VMAlloc *inside;

			// Arrange so that we will be notified about writes to the contents of this block. Clears 'fUpdated' flag.
			void watchWrites();
		};

	}
}

#endif
