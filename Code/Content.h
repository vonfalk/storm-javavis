#pragma once
#include "Core/TObject.h"

namespace code {

	STORM_PKG(core.asm);

	class RefSource;

	/**
	 * Content. Provides a pointer and a size which can be referred to by RefSources, and by
	 * extension, References. Only one RefSource can refer to a Content object.
	 */
	class Content : public ObjectOn<Compiler> {
		STORM_CLASS;
	public:
		STORM_CTOR Content();

		// Set the address of this content.
		void set(const void *address, Nat size);

		// Set the address of the content to an offset.
		void setOffset(Nat offset);

		// Get last address and size.
		virtual const void *address() const;
		virtual nat size() const;

		// Get the name of the owning RefSource (if any).
		MAYBE(Str *) STORM_FN ownerName() const;
	private:
		friend class RefSource;

		// Note: There are two ways addresses are stored depending on whether they are pointers or not:
		// If the address is a pointer, it is stored inside 'lastAddress'.
		// If it is an offset, it is stored inside 'lastOffset'. Then 'lastAddress' is null.

		// Last address and size.
		UNKNOWN(PTR_GC) const void *lastAddress;
		Nat lastOffset;
		Nat lastSize;

		// Owning RefSource.
		RefSource *owner;
	};

	/**
	 * Static content.
	 */
	class StaticContent : public Content {
		STORM_CLASS;
	public:
		StaticContent(const void *ptr);
		StaticContent(Nat offset);
	};

}
