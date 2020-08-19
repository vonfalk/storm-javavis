#pragma once
#include "Core/TObject.h"
#include "Core/WeakSet.h"
#include "Content.h"

namespace code {
	STORM_PKG(core.asm);

	class Reference;

	/**
	 * A RefSource is a static name that references can refer to. The RefSource also has some
	 * contents that provides the actual pointer that references refer to. While a RefSource is
	 * static in nature, the content can be altered freely. Doing so causes all references referring
	 * to the RefSource to be updated.
	 *
	 * The RefSource has a unique identifier of some sort. This is used in two situations:
	 * 1: It is presented to the user whenever a Listing is printed.
	 * 2: It is used as a unique identifier when parts of a program is saved on disk to be able
	 *    to link the program properly at a later time.
	 *
	 * The identifier is not provided by the RefSource class itself. Instead, implementations should
	 * create a subclass that implements 'title' for the identifier presented to the user and 'name'
	 * for the identifier used during serialization (not implemented yet).
	 */
	class RefSource : public ObjectOn<Compiler> {
		STORM_ABSTRACT_CLASS;
		friend class Reference;
	public:
		STORM_CTOR RefSource();
		STORM_CTOR RefSource(Content *content);

		// Set the content of this source. This updates all references referring to this source.
		void STORM_FN set(Content *to);

		// Clear any content in here.
		void STORM_FN clear();

		// Set to a static pointer (uses StaticContent).
		void setPtr(const void *to);

		// Make it so that all references in 'from' refers to this instance instead. Keeps 'from'
		// updated until there are no more 'Ref's referring to it (Reference instances are updated
		// immediately).
		void STORM_FN steal(RefSource *from);

		// Get the current address.
		inline const void *address() const {
			return cont ? cont->address() : null;
		}

		// Get content.
		inline Content *STORM_FN content() const { return cont; }

		// Get our title.
		virtual Str *STORM_FN title() const ABSTRACT;

		// Force update.
		void update();

		// Compute the RefSource to use. Usually returns this object, but if references from here
		// have been stolen, returns the new owner. Used to update any lingering weak references.
		RefSource *findActual();

	protected:
		// Output.
		virtual void STORM_FN toS(StrBuf *to) const;

	private:
		// Content.
		Content *cont;

		// All non Ref-references referring to this source.
		WeakSet<Reference> *refs;
	};


	/**
	 * RefSource labeled by a string.
	 */
	class StrRefSource : public RefSource {
		STORM_CLASS;
	public:
		StrRefSource(const wchar *name);
		STORM_CTOR StrRefSource(Str *name);
		STORM_CTOR StrRefSource(Str *name, Content *content);

		// Title.
		virtual Str *STORM_FN title() const;

	private:
		// Name.
		Str *name;
	};

}
