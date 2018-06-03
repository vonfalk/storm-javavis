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

		// Set the address of this content. Don't do this after the content has been added to a
		// RefSource.
		void set(const void *address, nat size);

		// Get last address and size.
		virtual const void *address() const;
		virtual nat size() const;

		// Get the name of the owning RefSource (if any).
		MAYBE(Str *) STORM_FN ownerName() const;
	private:
		friend class RefSource;

		// Last address and size.
		UNKNOWN(PTR_GC) const void *lastAddress;
		nat lastSize;

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
	};

}
