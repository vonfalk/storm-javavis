#pragma once

#if STORM_GC == STORM_GC_SMM

#include "VMAlloc.h"

namespace storm {
	namespace smm {

		/**
		 * A set designed to store generation numbers efficiently.
		 *
		 * Used similarly to an AddrSet, but stores generation numbers rather than ranges of
		 * addresses. This is useful when we're interested in quickly determining if a particular
		 * block contains any interesting pointers that needs scanning. If we're only interested in
		 * the generations, this approach is better than pointer summaries, as there is a fixed
		 * upper bound of generations (currently, 64) and we are thus able to use a compact and
		 * exact representation.
		 */
		class GenSet {
		public:
			// Create an empty set.
			GenSet() : data(0) {}

			// Create a set with one element.
			explicit GenSet(byte id) : data(0) {
				add(id);
			}

			// Clear.
			void clear() {
				data = 0;
			}

			// Add a generation.
			void add(byte gen) {
				data |= mask(gen);
			}

			// Add the contents of another set.
			void add(GenSet s) {
				data |= s.data;
			}

			// Remove a generation.
			void remove(byte gen) {
				data &= ~mask(gen);
			}

			// Remove the contents of another set.
			void remove(GenSet s) {
				data &= ~s.data;
			}

			// Contains a particular generaion?
			bool has(byte gen) const {
				return (data & mask(gen)) != 0;
			}

			// Contains at least one element from another set?
			bool has(GenSet s) const {
				return (data & s.data) != 0;
			}

			// Is the set empty?
			bool empty() const {
				return data == 0;
			}
			bool any() const {
				return data != 0;
			}

			// Maximum number of generations supported. This is the smallest of the number of bits
			// in a size_t and the number of generations addressable by the bits provided by VMAlloc.
			static const size_t maxGen =
				(sizeof(size_t)*CHAR_BIT) < (size_t(1) << identifierBits) ?
														  (sizeof(size_t)*CHAR_BIT) :
														  (size_t(1) << identifierBits);

		private:
			// Mask for the generations.
			static const size_t genMask = maxGen - 1;

			// The one and only value we need.
			size_t data;

			// Create a mask for a particular generation, or zero if none exists.
			static inline nat64 mask(byte gen) {
				return nat64(gen < maxGen) << (gen & genMask);
			}
		};

		wostream &operator <<(wostream &to, GenSet genSet);


	}
}

#endif
