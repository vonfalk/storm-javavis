#pragma once

#if STORM_GC == STORM_GC_SMM

namespace storm {
	namespace smm {

		// Insert an element into a sorted vector. This should be slightly better than using
		// lower_bound followed by insert, since we don't have to traverse the vector twice, but we
		// "resize" the vector as we're finding the proper location. Assumes that T is a type that's
		// cheap to copy, as we will copy instances around quite a lot.
		template <class T, class Compare>
		size_t insertSorted(vector<T> &into, const T &insert, const Compare &less) {
			// Insert the element to make it larger. This might not be the correct location.
			into.push_back(insert);

			// Find the proper location!
			for (size_t i = into.size() - 1; i > 0; i--) {
				if (less(into[i - 1], insert)) {
					// We found the proper location. Put it there, and we're done!
					into[i] = insert;
					return i;
				}

				// Not correct yet. Move elements along.
				into[i] = into[i - 1];
			}

			// If we did not terminate early, we should move it to the first location!
			into[0] = insert;
			return 0;
		}

		template <class T>
		size_t insertSorted(vector<T> &into, const T &insert) {
			return insertSorted(into, insert, std::less<T>());
		}

	}
}

#endif
