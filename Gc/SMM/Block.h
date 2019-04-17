#pragma once

#if STORM_GC == STORM_GC_SMM

#include "GenSet.h"
#include "VMAlloc.h"
#include "Gc/Format.h"

namespace storm {
	namespace smm {

		class VMAlloc;

		/**
		 * A block of memory allocated in some generation with some associated metadata used by the
		 * generation for bookkeeping. The Block class represents the header in the beginning of an
		 * allocation, and the remainder of the allocation is usable to store objects.
		 *
		 * We assume memory inside the blocks are allocated from low addresses to high addresses
		 * simply by bumping a pointer, and the Block keeps track of this. Blocks are never moved,
		 * even though objects inside a block may move inside or between blocks. However,
		 * generations may chose to re-create a block around previously allocated data in cases
		 * where some allocations may not be moved. This is, however, considered to be a new block
		 * without any connection to the previous block.
		 */
		class Block {
		public:
			// Create.
			Block(size_t size) : size(size), committed(0), reserved(0), flags(0) {}

			// Current size (excluding the block itself).
			const size_t size;

			// Amount of memory committed (excluding the block itself).
			size_t committed;

			// Amount of memory reserved. 'reserved >= committed'. Memory that is reserved but not
			// committed is being initialized, and can not be assumed to contain usable data.
			size_t reserved;

			// Various flags for this block.
			enum Flags {
				// This block is empty and can be deallocated.
				fEmpty = 0x01,

				// This block is in use by an allocator.
				fUsed = 0x02,
			};

			// Modify flags.
			inline bool hasFlag(Flags flag) {
				return (atomicRead(flags) & flag) != 0;
			}
			inline void setFlag(Flags flag) {
				atomicOr(flags, flag);
			}
			inline void clearFlag(Flags flag) {
				atomicAnd(flags, ~flag);
			}

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

			// Is it possible that this block contains references to a generation with the specified
			// number?
			bool mayReferTo(byte gen) const {
				// TODO: Check stale memory!
				return true;

				return summary.has(gen);
			}

			// Scan all objects in this block using the supplied scanner.
			template <class Scanner>
			typename Scanner::Result scan(typename Scanner::Source &source) {
				// TODO: Perhaps we should update our summary here?
				return fmt::Scan<Scanner>::objects(source, mem(fmt::headerSize), mem(fmt::headerSize + committed));
			}

			// Scan all objects that fulfill a specified predicate using the supplied scanner.
			template <class Predicate, class Scanner>
			typename Scanner::Result scanIf(const Predicate &predicate, typename Scanner::Source &source) {
				void *at = mem(fmt::headerSize);
				void *limit = mem(fmt::headerSize + committed);

				while (at < limit) {
					void *next = fmt::skip(at);
					if (predicate(at, (char *)next - fmt::headerSize)) {
						typename Scanner::Result r = fmt::Scan<Scanner>::objects(source, at, fmt::skip(at));
						if (r)
							return r;
					}

					at = next;
				}

				return typename Scanner::Result();
			}

			// Verify the contents of this block.
			void dbg_verify();

		private:
			// No copying!
			Block(const Block &o);
			Block &operator =(const Block &o);

			// A summary of the references contained in here.
			GenSet summary;

			// Our flags (updated atomically by the corresponding functions).
			size_t flags;

			// Arrange so that we will be notified about writes to the contents of this block. Clears 'fUpdated' flag.
			void watchWrites();
		};

	}
}

#endif
