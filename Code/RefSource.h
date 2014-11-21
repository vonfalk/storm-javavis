#pragma once

namespace code {

	class Arena;

	// Represents the source of a reference, ie. the thing in the system
	// we are referring. The RefSource must provide an address to be referenced
	// along with a user-readable string that identifies the object. This identifier
	// is supposed to be unique, since it is used to resolve any external references
	// when loading code from a file.
	class RefSource : NoCopy {
	public:
		RefSource(Arena &arena, const String &title);
		~RefSource();

		// The arena we belong to.
		Arena &arena;

		// Set the address we're currently referring.
		void set(void *address, nat size = 0);

		// Get the last address.
		inline void *address() const { return lastAddress; }

		inline const String &getTitle() const { return title; }
		inline nat getId() const { return referenceId; }

	private:
		// Our reference id. This is our unique identifier in the system.
		nat referenceId;

		// Our title.
		String title;

		// Last known address.
		void *lastAddress;
	};

}
