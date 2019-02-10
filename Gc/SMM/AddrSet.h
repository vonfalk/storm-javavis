#pragma once

#if STORM_GC == STORM_GC_SMM

#include "Utils/Bitwise.h"

namespace storm {
	namespace smm {


		/**
		 * Describes a set of addresses, at some resolution. Usable to keep track of pointers that
		 * do not necessarily point to the beginning of an object.
		 *
		 * Represents some contiguous range of addresses, and supports marking addresses
		 * therein. However, the representation only keeps track of marked addresses down to some
		 * resolution. Otherwise, the memory requirements would be prohibitively large for a large
		 * range of addresses. Since the structure does not keep track of exactly which addresses
		 * are marked, it is not possible to unmark addresses in a reliable fashion, which is why
		 * the structure does not allow this operation.
		 *
		 * The total number of distinct regions that the structure keeps track of is specified as
		 * "minBytes". The address range and thereby also the resolution can be configured through
		 * parameters to the constructor. Note that the address range will be rounded up to the
		 * nearest power of two for efficiency.
		 */
		template <size_t minBytes>
		class AddrSet {
			static const size_t totalBytes = NextPowerOfTwo<minBytes>::value;
		public:
			// Create empty.
			AddrSet(size_t from, size_t to) {
				size_t offset = from & ~size_t(0x3F);
				size_t len = nextPowerOfTwo(to - offset);
				size_t shift = trailingZeros(len);
				size_t range = trailingZeros(totalBytes * CHAR_BIT);

				// Make sure we actually get some usable bits. Maximum resolution is one bit per
				// byte in the memory.
				shift -= min(shift, range);

				// Encode.
				data = offset | shift;

				clear();
			}

			// Clear.
			void clear() {
				memset(marked, 0, totalBytes);
			}

			// Add an address.
			void add(size_t addr) {
				size_t id = (addr - offset()) >> shift();
				marked[(id / CHAR_BIT) & (totalBytes - 1)] |= (id < totalBytes) << (id % CHAR_BIT);
			}

			// Test if an address is set.
			bool test(size_t addr) const {
				size_t id = (addr - offset()) >> shift();
				// Make sure to return false if "addr" is out of bounds.
				return (id < totalBytes)
					& ((marked[(id / CHAR_BIT) & (totalBytes - 1)] & (1 << (id % CHAR_BIT))) != 0);
			}

		private:
			// Data. Contains an offset and a shift (the lowest 6 bits).
			size_t data;

			// Get the offset.
			inline size_t offset() {
				return data & ~size_t(0x3F);
			}

			// Get the shift.
			inline size_t shift() {
				return data & size_t(0x3f);
			}

			// Marked regions.
			byte marked[totalBytes];
		};

	}
}

#endif
