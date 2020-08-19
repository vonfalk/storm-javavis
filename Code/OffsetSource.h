#pragma once
#include "Core/TObject.h"
#include "Core/WeakSet.h"
#include "Size.h"

namespace code {
	STORM_PKG(core.asm);

	class OffsetReference;

	/**
	 * A RefSource that provides an offset rather than a pointer.
	 *
	 * The main reason the two are kept separate is to make sure that pointer-references are not
	 * accidentally treated as offset-references, thus causing confusion for the garbage collector
	 * (one needs to be scanned, the other does not). Furthermore, we allow for offset-references to
	 * not refere to anything (they are simply zero), while pointer-references are not.
	 *
	 * Additionally, OffsetRefSource objects do not have a Content, since they are simply an
	 * offset. These offsets may not refer other things, as is the case with pointer-references.
	 */
	class OffsetSource : public ObjectOn<Compiler> {
		STORM_ABSTRACT_CLASS;
		friend class OffsetReference;
	public:
		STORM_CTOR OffsetSource();
		STORM_CTOR OffsetSource(Offset offset);

		// Set the offset.
		void STORM_FN set(Offset offset);

		// Make it so that all references in 'from' refers to this instance instead. Keeps 'from'
		// updated until there are no more 'Ref's referring to it (Reference instances are updated
		// immediately).
		void STORM_FN steal(OffsetSource *from);

		// Get the current offset.
		Offset offset() const;

		// Get our title.
		virtual Str *STORM_FN title() const ABSTRACT;

		// Force update.
		void update();

		// Get actual source, in case content was stolen.
		OffsetSource *STORM_FN findActual();

	protected:
		// Output.
		virtual void STORM_FN toS(StrBuf *to) const;

	private:
		// Data.
		Offset data;

		// All non-Ref references referring to this source.
		WeakSet<OffsetReference> *refs;

		// References stolen by.
		OffsetSource *stolenBy;
	};

	/**
	 * String-labeled OffsetSource.
	 */
	class StrOffsetSource : public OffsetSource {
		STORM_CLASS;
	public:
		StrOffsetSource(const wchar *name);
		STORM_CTOR StrOffsetSource(Str *name);
		STORM_CTOR StrOffsetSource(Str *name, Offset offset);

		// Title.
		virtual Str *STORM_FN title() const;

	private:
		// Name.
		Str *name;
	};

}
