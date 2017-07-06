#pragma once
#include "Core/TObject.h"
#include "Core/WeakSet.h"

namespace code {
	STORM_PKG(core.asm);

	class RefSource;
	class Reference;

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

	/**
	 * Something that references can refer to. A RefSource has some kind of unique identifier that
	 * can be stored whenever listings are serialized, so that we can properly link the program from
	 * a previously serialized representation. Currently, this is a String, but we want to replace
	 * this with a more generic object later on.
	 *
	 * A RefSource is something very static, but its contents can be replaced. When that happens,
	 * all references will be updated.
	 */
	class RefSource : public ObjectOn<Compiler> {
		STORM_CLASS;
		friend class Reference;
	public:
		RefSource(const wchar *title);
		STORM_CTOR RefSource(Str *title);
		STORM_CTOR RefSource(Str *title, Content *content);

		// Set the content of this source. This updates all references referring to this source.
		void STORM_FN set(Content *to);

		// Set to a static pointer (uses StaticContent).
		void setPtr(const void *to);

		// Get the current address.
		inline const void *address() const {
			return cont ? cont->address() : null;
		}

		// Get content.
		inline Content *STORM_FN content() const { return cont; }

		// Get our title.
		inline Str *STORM_FN title() const { return name; }

		// Output.
		virtual void STORM_FN toS(StrBuf *to) const;

		// Force update.
		void update();

	private:
		// Name.
		Str *name;

		// Content.
		Content *cont;

		// All non Ref-references referring to this source.
		WeakSet<Reference> *refs;
	};

}
