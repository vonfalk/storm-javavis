#pragma once

#include "Utils/HashMap.h"
#include "Utils/InlineSet.h"
#include "Reference.h"

namespace code {

	class RefSource;

	// The class responsible for managing all references withing a single arena.
	// This class is tightly coupled with the Arena, and is not designed to be
	// used outside the arena.
	class RefManager : NoCopy {
	public:
		RefManager();
		~RefManager();

		static const nat invalid;

		nat addSource(RefSource *source);
		void removeSource(nat id);
		void setAddress(nat id, void *address, nat size);

		// Get the source that is currently associated with the address at "addr". Returns "invalid" if none exists.
		// Works under the assumption that no sources refer overlapped chunks of memory.
		nat ownerOf(void *addr) const;

		// Get the current address of a source.
		void *address(nat id) const;
		// Get the name of RefSource "id".
		const String &name(nat id) const;
		// Get the RefSource of "id".
		RefSource *source(nat id) const;

		// Reference counting used for the Ref class (light references).
		void addLightRef(nat id);
		void removeLightRef(nat id);

		// Reference management for the Reference class.
		void *addReference(Reference *r, nat id);
		void removeReference(Reference *r, nat id);

		// Disable checking of dead references (during shutdown).
		void preShutdown();

	private:
		struct Info {
			RefSource *source;

			// The size and position of the used data.
			void *address;
			nat size;

			// Number of light references
			nat lightCount;

			// All "hard" references
			util::InlineSet<Reference> references;
		};

		// Map from indices to info-structs.
		typedef hash_map<nat, Info *> InfoMap;
		InfoMap infoMap;

		// First possible free index.
		nat firstFreeIndex;

		// Map containing current addresses mapping to indices.
		typedef std::multimap<void *, nat> AddrMap;
		AddrMap addresses;

		// Are we shutting down?
		bool shutdown;

		// Register an address.
		void addAddr(void *addr, nat id);

		// Remove an address.
		void removeAddr(void *addr, nat id);
	};

}
