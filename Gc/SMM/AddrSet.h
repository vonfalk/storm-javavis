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
		public:
			static const size_t totalBytes = NextPowerOfTwo<minBytes>::value;

			// Create empty.
			AddrSet(void *from, void *to) {
				init(size_t(from), size_t(to));
			}
			AddrSet(size_t from, size_t to) {
				init(from, to);
			}

			// Clear.
			void clear() {
				for (size_t i = 0; i < totalBytes; i++)
					marked[i] = 0;
			}

			// Add an address.
			void add(void *addr) { add(size_t(addr)); }
			void add(size_t addr) {
				size_t id = (addr - offset()) >> shift();
				marked[(id / CHAR_BIT) & (totalBytes - 1)] |= (id < totalBytes * CHAR_BIT) << (id % CHAR_BIT);
			}

			// Test if an address is set.
			bool has(void *addr) const { return test(size_t(addr)); }
			bool has(size_t addr) const {
				size_t id = (addr - offset()) >> shift();
				// Make sure to return false if "addr" is out of bounds.
				return (id < totalBytes * CHAR_BIT)
					& ((marked[(id / CHAR_BIT) & (totalBytes - 1)] & (1 << (id % CHAR_BIT))) != 0);
			}

			// Get the offset.
			inline size_t offset() const {
				return data & ~size_t(0x3F);
			}

			// Get the shift.
			inline size_t shift() const {
				return data & size_t(0x3f);
			}

		private:
			// Data. Contains an offset and a shift (the lowest 6 bits).
			size_t data;

			// Marked regions.
			byte marked[totalBytes];

			// Initialize.
			void init(size_t from, size_t to) {
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
		};

		template <size_t sz>
		wostream &operator <<(wostream &out, const AddrSet<sz> &s) {
			size_t from = s.offset();
			size_t bitSz = size_t(1) << s.shift();
			size_t to = s.offset() + bitSz * s.totalBytes * CHAR_BIT;

			out << (void *)from << L" ";
			for (size_t i = from; i < to; i += bitSz)
				out << (s.has(i) ? '+' : '-');
			out << L" " << (void *)to << L" (resolution: " << bitSz << L" bytes)";
			return out;
		}

	}
}

#endif
