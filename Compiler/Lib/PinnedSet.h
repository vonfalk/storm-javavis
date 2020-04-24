#pragma once
#include "Core/Object.h"
#include "Core/Array.h"
#include "Gc/Gc.h"

namespace storm {
	STORM_PKG(core.unsafe);

	/**
	 * A set of pinned pointers.
	 *
	 * These pointers are scanned in the same way objects on the stack are scanned. As such, these
	 * pointers may point inside objects contrary to all other pointers in the system.
	 *
	 * It is then possible to query the set of pointers with a unsafe.RawPtr to see if this object
	 * contains a pointer to somewhere inside the specified RawPtr, and if so, at which offset. This
	 * can be done at O(log(n)), except for the first query, which may take O(nlog(n))
	 *
	 * This class is useful when collecting data from a running program in order for later
	 * instrumentation.
	 *
	 * This class is implemented as a flat array of objects that is sorted on demand.
	 */
	class PinnedSet : public Object {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR PinnedSet();

		// Copy.
		PinnedSet(const PinnedSet &o);

		// Destroy.
		~PinnedSet();

		// Deep copy (does not do anything).
		virtual void STORM_FN deepCopy(CloneEnv *env);

		// Add a pointer to an object. The pointer may not necessarily point to the start of the object.
		void CODECALL add(void *ptr);

		// Check if an object is present in here. This pointer must be to the start of an
		// allocation. Returns true if this object contains a pointer to somewhere inside the
		// provided object.
		Bool CODECALL has(const void *query);

		// Same as 'has', but generates all offsets as an array.
		Array<Nat> *CODECALL offsets(const void *query);

		// Clear the set.
		void STORM_FN clear();

	private:
		// Data.
		struct Data {
			// Number of elements.
			size_t count;

			// Number of filled elements.
			size_t filled;

			// Sorted?
			bool sorted;

			// Actual data.
			void *v[1];
		};

		// Data.
		UNKNOWN(PTR_NOGC) Data *data;

		// Root for the data.
		UNKNOWN(PTR_NOGC) Gc::Root *root;

		// Reserve size for at least 'n' elements.
		void reserve(size_t n);

		// Sort the array, and remove any duplicates.
		void sort();

		// Size of data struct for 'n' elements.
		static size_t dataSize(size_t n);
	};

}
