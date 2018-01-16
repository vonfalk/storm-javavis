#pragma once
#include "Gc.h"
#include "Utils/Lock.h"

namespace storm {

	/**
	 * Table that keeps track of code allocations in the system. Allows searching for the allocation
	 * that contains a certain address in approximately O(log n) time (sometimes worse). The table
	 * will keep a weak reference to the code allocations, so no 'free' operation is required.
	 *
	 * Note: the metadata of code allocations are not considered to be a part of the code.
	 *
	 * Allocated from within the Engine when it is needed.
	 */
	class CodeTable : NoCopy {
	public:
		// Create.
		CodeTable(Engine &e);

		// Destroy.
		~CodeTable();

		// Add code allocation.
		void add(void *code);

		// Get the start of a code allocation contained in here. Returns null on failure.
		void *find(void *pos);

		// Clear the table. Also destroys the GC root we are using.
		void clear();

	private:
		// Owner.
		Engine &e;

		// Lock.
		util::Lock lock;

		// Pointer data. We allocate a root for these.
		struct Data {
			// The actual table of all functions. Sorted from low to high addresses. null pointers
			// are at the end of the array.
			GcWeakArray<void> *table;

			// Watch to see if some addresses inside table has changed.
			GcWatch *watch;
		};

		// The data.
		Data data;

		// Current root. Null until we allocate something inside 'table' and 'watch'.
		Gc::Root *root;

		// The current number of elements (approx). The index 'count' is always either 'null' or
		// just outside the array.
		Nat count;

		// Do we think the table is sorted at the moment? Even if it is 'true', the table might not
		// be sorted anymore due to the GC moving objects around.
		bool sorted;

		// Ensure we have at least 'count' elements in the array. Note: 'count' may shrink during an
		// 'ensure' operation since some objects could have been erased since the last time we looked.
		void ensure(Nat count);

		// Compact the from 'src' into 'data.table'. 'src' may be equal to 'data.table'. Updates
		// 'data.watch' as well.
		void compact(GcWeakArray<void> *src);
	};

}
