#pragma once

namespace storm {

	/**
	 * Memory summary provided by GC implementations.
	 *
	 * TODO: Perhaps we shall move this into Core/ so that Storm can use it?
	 */
	class MemorySummary {
	public:
		MemorySummary();

		// Total number of bytes in use by objects.
		size_t objects;

		// Number of bytes currently unusable due to fragmentation or other issues.
		size_t fragmented;

		// Number of bytes used for internal data structures.
		size_t bookkeeping;

		// Number of bytes that are allocated from the OS but not currently in use.
		size_t free;

		// Total number of bytes allocated from the OS.
		size_t allocated;

		// Total number of bytes reserved from the OS.
		size_t reserved;
	};

	// Output.
	wostream &operator <<(wostream &to, const MemorySummary &o);

}
