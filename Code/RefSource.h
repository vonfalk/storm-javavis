#pragma once

namespace code {

	class Arena;

	/**
	 * Base class for contents that can be viewed through a RefSource. Content is more or less only
	 * a pointer and an optional size. The Content itself also knows (through the RefManager) which
	 * References it contains.
	 *
	 * The intention is that this class is subclasses with new content as needed.
	 */
	class Content : NoCopy {
	public:
		Content(Arena &arena);
		~Content();

		// The arena we belong to.
		Arena &arena;

		// Set the address (and optionally the size) we are representing.
		void set(void *address, nat size = 0);

		// Get the last address.
		inline void *address() const { return lastAddress; }
		inline nat size() const { return lastSize; }

	private:
		// Last address and size.
		void *lastAddress;
		nat lastSize;
	};

	/**
	 * Static content.
	 */
	class StaticContent : public Content {
	public:
		StaticContent(Arena &arena, void *ptr);
	};

	/**
	 * Represents the source of a reference, ie. the thing in the system we are referring. The
	 * RefSource itself provides an address to its Content (see above) along with an optional
	 * size. The RefSource also contains a user-defined string that identifies the content (will be
	 * replaced by something else later on) so that references can be printed nicely.
	 */
	class RefSource : NoCopy {
	public:
		RefSource(Arena &arena, const String &title);
		~RefSource();

		// Clear this (equivalent to running the destructor early). Usually not needed.
		void clear();

		// The arena we belong to.
		Arena &arena;

		// Set the address we're currently referring. The reference management takes ownership
		// of this content after 'set' is called.
		void set(Content *content);

		// Set to a static address (uses StaticContent).
		void setPtr(void *ptr);

		// Get the last address.
		inline void *address() const { return content ? content->address() : null; }

		// Get the content.
		inline Content *contents() const { return content; }

		const String &title() const;
		inline nat getId() const { return referenceId; }

	private:
		// Our reference id. This is our unique identifier in the system.
		nat referenceId;

		// Current content.
		Content *content;
	};


}
