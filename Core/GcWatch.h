#pragma once

namespace storm {

	/**
	 * Location dependency declarations for objects in the GC.
	 *
	 * This object encapsulates an interface which enables users to query if an object has been
	 * moved by the GC since the last time it was accessed. This is useful when implementing hash
	 * maps for which the object's location is its key into the table. Moving the object alters this
	 * identity, and therefore destroys the hash map. Using a GcWatch to keep track of changes
	 * allows the map to detect these conditions and act accordingly.
	 *
	 * Note: Call runtime::createWatch() to create GcWatch objects.
	 */
	class GcWatch : NoCopy {
	public:
		// Add an address to be watched.
		virtual void add(const void *addr) = 0;

		// Remove an address from the watchlist.
		virtual void remove(const void *addr) = 0;

		// Clear all watched addresses.
		virtual void clear() = 0;

		// See if any object has moved. May return false positives.
		virtual bool moved() = 0;

		// See if the object has moved. May return false positives.
		virtual bool moved(const void *addr) = 0;

		// See if this object is tagged, i.e. making 'moved' always return true.
		virtual bool tagged() = 0;

		// Clone this GcWatch (and its current state).
		virtual GcWatch *clone() const = 0;
	};

}
