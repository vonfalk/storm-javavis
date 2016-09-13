#pragma once
#include "RefSource.h"

namespace code {
	STORM_PKG(core.asm);

	/**
	 * There are two kind of references: A lightweight one and a more robust one.
	 *
	 * The lightweight one is fast but does not track who is referencing a RefSource. Therefore,
	 * this is to be used mainly when passing references as parameters.
	 *
	 * The more robust one is slightly slower, but tracks who is using something and gets notified
	 * whenever the reference is updated.
	 */

	/**
	 * Robust reference.
	 */
	class Reference : public ObjectOn<Compiler> {
		STORM_CLASS;
		friend class Ref;
	public:
		// First parameter is what the reference should refer to, second is who is referring.
		STORM_CTOR Reference(RefSource *to, Content *inside);
		STORM_CTOR Reference(Ref to, Content *inside);

		// Get the content we're associated with, ie. who's referring to something.
		inline Content *referrer() const { return owner; }

		// Notification of changed address.
		virtual void moved(const void *newAddr);

		// Get current address.
		inline const void *address() const { return to->address(); }

		// ToS.
		virtual void STORM_FN toS(StrBuf *to) const;
	private:
		// Owner = referrer
		Content *owner;

		// Who are we referring to?
		RefSource *to;
	};

	/**
	 * Weak reference.
	 */
	class Ref {
		STORM_VALUE;
		friend class Reference;
		friend wostream &operator <<(wostream &to, const Ref &r);
		friend StrBuf &operator <<(StrBuf &to, Ref r);
	public:
		STORM_CTOR Ref(RefSource *to);
		STORM_CTOR Ref(Reference *ref);
		STORM_CTOR Ref(const Ref &o);

		void deepCopy(CloneEnv *env);

		inline Bool STORM_FN operator ==(const Ref &o) const { return to == o.to; }
		inline Bool STORM_FN operator !=(const Ref &o) const { return to != o.to; }

		// Get the address.
		inline const void *address() const { return to->address(); }

		// Get the name of our target.
		inline Str *title() const { return to->title(); }

	private:
		// Referring to:
		RefSource *to;
	};

	wostream &operator <<(wostream &to, const Ref &r);
	StrBuf &STORM_FN operator <<(StrBuf &to, Ref r) ON(Compiler);

}
