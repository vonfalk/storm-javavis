#pragma once

#if STORM_GC == STORM_GC_SMM

#include "Utils/Bitwise.h"
#include "Config.h"

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
		 *
		 * TODO: Is it better to work with nat:s or size_t:s rather than bytes?
		 */
		template <size_t minBytes>
		class AddrSet {
		public:
			// Number of bytes.
			static const size_t totalBytes = NextPowerOfTwo<minBytes>::value;

			// Bits per element in the array.
			static const size_t elemBits = CHAR_BIT;

			// Create empty.
			AddrSet() {
				init(0, 1);
			}
			AddrSet(const void *from, const void *to) {
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

			// Empty?
			bool empty() const {
				size_t d = 0;
				for (size_t i = 0; i < totalBytes; i++)
					d |= marked[i];
				return d == 0;
			}

			// Add an address.
			void add(const void *addr) {
				add(size_t(addr));
			}
			void add(size_t addr) {
				size_t id = (addr - offset()) >> shift();
				marked[(id / elemBits) & (totalBytes - 1)] |= (id < totalBytes * elemBits) << (id % elemBits);
			}

			// Add a range of addresses.
			void add(const void *from, const void *to) {
				add(size_t(from), size_t(to));
			}
			void add(size_t from, size_t to) {
				size_t off = offset();
				size_t ext = off + size();

				// Completely outside our range?
				if (from >= ext || to <= off)
					return;

				// Note: We don't need min and max on both, since we know that [from, to[ overlaps our range.
				from = max(from, off);
				to = min(to, ext) - 1;

				size_t idF = (from - offset()) >> shift();
				size_t idT = (to - offset()) >> shift();

				size_t chF = idF / elemBits;
				byte bitF = idF % elemBits;
				size_t chT = idT / elemBits;
				byte bitT = idT % elemBits;

				if (chF == chT) {
					// Same chunk. Make a mask with a series of ones.
					byte mask = (2 << bitT) - 1;
					mask &= ~byte((1 << bitF) - 1);
					marked[chF] |= mask;
				} else {
					// Different chunks. Check multiple bits in a range of slots.

					// All bits in the first chunk, from the specified bit and upward.
					marked[chF] |= ~byte((1 << bitF) - 1);
					// All bits in the chunks between.
					for (size_t ch = chF + 1; ch < chT; ch++)
						marked[ch] |= std::numeric_limits<size_t>::max();
					// All bits in the last chunk, from the specified bit and downward.
					marked[chT] |= ((2 << bitT) - 1);
				}
			}

			// Test if an address is set.
			bool has(const void *addr) const { return has(size_t(addr)); }
			bool has(size_t addr) const {
				size_t id = (addr - offset()) >> shift();
				// Make sure to return false if "addr" is out of bounds.
				return (id < totalBytes * elemBits)
					& ((marked[(id / elemBits) & (totalBytes - 1)] & (1 << (id % elemBits))) != 0);
			}

			// See if any address in the range [from, to[ is set.
			bool has(const void *from, const void *to) const {
				return has(size_t(from), size_t(to));
			}
			bool has(size_t from, size_t to) const {
				size_t off = offset();
				size_t ext = off + size();

				// Completely outside our range?
				if (from >= ext || to <= off)
					return false;

				// Note: We don't need min and max on both, since we know that [from, to[ overlaps our range.
				from = max(from, off);
				to = min(to, ext) - 1;

				size_t idF = (from - offset()) >> shift();
				size_t idT = (to - offset()) >> shift();

				size_t chF = idF / elemBits;
				byte bitF = idF % elemBits;
				size_t chT = idT / elemBits;
				byte bitT = idT % elemBits;

				if (chF == chT) {
					// Same chunk. Make a mask with a series of ones.
					byte mask = (2 << bitT) - 1;
					mask &= ~byte((1 << bitF) - 1);
					return (marked[chF] & mask) != 0;
				} else {
					// Different chunks. Check multiple bits in a range of slots.

					// All bits in the first chunk, from the specified bit and upward.
					byte r = marked[chF] & ~byte((1 << bitF) - 1);
					// All bits in the chunks between.
					for (size_t ch = chF + 1; ch < chT; ch++)
						r |= marked[ch];
					// All bits in the last chunk, from the specified bit and downward.
					r |= marked[chT] & ((2 << bitT) - 1);

					return r != 0;
				}
			}

			// Traverse all set regions in the set. Calls 'fn' with two size_t values for each set
			// range of addresses. If 'fn' returns 'false', the traversal is aborted. Returns 'true'
			// if completed entirely or 'false' if aborted.
			template <class Fn>
			inline bool traverse(Fn &fn) const {
				// Are we looking for a negative? Either 0 or 0xFF (used to flip bits conditionally)
				byte flip = 0;

				// Start of positive range.
				size_t start = 0;

				// Next bit to examine in the bitmask.
				size_t at = 0;
				while (at < totalBytes * elemBits) {
					size_t atElem = at / elemBits;
					byte atBit = at % elemBits;

					// Check if this element has any bits set.
					byte current = marked[atElem] ^ flip;
					current &= ~byte((1 << atBit) - 1);

					if (current) {
						// Find the lowest set bit.
						nat next = trailingZeros(current);
						at = atElem*elemBits + next;

						if (flip) {
							if (!fn(toAddr(start), toAddr(at)))
								return false;
						}
						start = at;
						flip = ~flip;
						at++;
					} else {
						// Advance to the next element and look there.
						at = (atElem + 1)*elemBits;
					}
				}

				if (flip)
					return fn(toAddr(start), toAddr(totalBytes * elemBits));
				else
					return true;
			}

			// See if this set intersects another set.
			template <size_t sz>
			inline bool intersects(const AddrSet<sz> &other) const {
				Intersect<sz> i(other);
				return !traverse(i);
			}

			// Resize to include a range of addresses.
			AddrSet resize(const void *from, const void *to) const {
				return resize(size_t(from), size_t(to));
			}
			AddrSet resize(size_t from, size_t to) const {
				from = min(from, offset());
				to = max(to, offset() + size());

				Copy c(from, to);
				traverse(c);
				return c.set;
			}

			// Get the offset.
			inline size_t offset() const {
				return data & ~size_t(0x3F);
			}

			// Get the shift.
			inline size_t shift() const {
				return data & size_t(0x3f);
			}

			// Get the total size covered.
			inline size_t size() const {
				return (size_t(1) << shift()) * totalBytes * elemBits;
			}

			// Check if an address is covered by this set.
			inline bool covers(const void *addr) const {
				return covers(size_t(addr));
			}
			inline bool covers(size_t addr) const {
				if (addr < offset())
					return false;
				if (addr - offset() >= size())
					return false;
				return true;
			}
			inline bool covers(const void *from, const void *to) const {
				return covers(size_t(from), size_t(to));
			}
			inline bool covers(size_t from, size_t to) const {
				if (from < offset())
					return false;
				if (to - offset() > size())
					return false;
				return true;
			}

			// Output as a string, but for a particular range of addresses.
			String toS(const void *from, const void *to) {
				return toS(size_t(from), size_t(to));
			}
			String toS(size_t from, size_t to) {
				std::wostringstream out;

				size_t bitSz = size_t(1) << shift();
				from = roundDown(from, bitSz);
				to = roundUp(to, bitSz);

				out << (void *)from << L" ";
				for (size_t i = from; i < to; i += bitSz)
					out << (has(i) ? '+' : '-');
				out << L" " << (void *)to << L" (resolution: " << bitSz << L" bytes)";

				return out.str();
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
				size_t range = trailingZeros(totalBytes * elemBits);

				// Make sure we actually get some usable bits. Maximum resolution is one bit per
				// byte in the memory.
				shift -= min(shift, range);

				// Encode.
				data = offset | shift;

				clear();
			}

			// Convert from bit indices to real addresses.
			size_t toAddr(size_t index) const {
				return offset() + (index << shift());
			}

			// Check intersections.
			template <size_t sz>
			struct Intersect {
				const AddrSet<sz> &other;

				Intersect(const AddrSet<sz> &other) : other(other) {}

				bool operator ()(size_t from, size_t to) const {
					return !other.has(from, to);
				}
			};

			// Copy to another AddrSet.
			struct Copy {
				AddrSet set;

				Copy(size_t from, size_t to) : set(from, to) {}

				bool operator ()(size_t from, size_t to) {
					set.add(from, to);
					return true;
				}
			};
		};

		// Summary type for the entire address space.
		typedef AddrSet<summaryBytes> AddrSummary;

		// Summary type for object pinning.
		typedef AddrSet<pinnedBytes> PinnedSet;

		template <size_t sz>
		wostream &operator <<(wostream &out, const AddrSet<sz> &s) {
			size_t from = s.offset();
			size_t bitSz = size_t(1) << s.shift();
			size_t to = s.offset() + s.size();

			out << (void *)from << L" ";
			for (size_t i = from; i < to; i += bitSz)
				out << (s.has(i) ? '+' : '-');
			out << L" " << (void *)to << L" (resolution: " << bitSz << L" bytes)";
			return out;
		}

	}
}

#endif
