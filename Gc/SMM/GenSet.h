#pragma once

#if STORM_GC == STORM_GC_SMM

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

		private:
			enum {
				// Maximum number of generations.
				maxGen = 64,

				// Mask for generations.
				genMask = maxGen - 1
			};

			// The one and only 64-bit value we need.
			nat64 data;

			// Create a mask for a particular generation.
			static inline nat64 mask(byte gen) {
				return nat64(1) << (gen & genMask);
			}
		};


	}
}

#endif
