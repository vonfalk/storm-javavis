#pragma once
#include "Content.h"
#include "OffsetSource.h"
#include "Size.h"

namespace code {
	STORM_PKG(core.asm);

	/**
	 * Offset references work similar to regular references (see Ref and Reference).
	 *
	 * The main difference is that OffsetRef and OffsetReference might not refer to any source,
	 * meaning they are simply the offset 0.
	 *
	 * Furthermore, these offsets also contain a regular offset, meaning that their final value is
	 * the sum of the contained offset and whatever offset the original offset is referring to.
	 */


	/**
	 * A weak reference to an offset. Might be 'null', meaning a zero offset.
	 */
	class OffsetRef {
		STORM_VALUE;
		friend class OffsetReference;
		friend class Operand;
		friend wostream &operator <<(wostream &to, const OffsetRef &r);
		friend StrBuf &operator <<(StrBuf &to, OffsetRef r);
	public:
		STORM_CTOR OffsetRef();
		STORM_CAST_CTOR OffsetRef(OffsetSource *to);
		STORM_CAST_CTOR OffsetRef(OffsetReference *ref);
		STORM_CTOR OffsetRef(OffsetSource *to, Offset offset);

		void STORM_FN deepCopy(CloneEnv *env);

		inline Bool STORM_FN operator ==(const OffsetRef &o) const {
			return to == o.to && delta == o.delta;
		}
		inline Bool STORM_FN operator !=(const OffsetRef &o) const {
			return !(*this == o);
		}

		// Add/subtract from the internal offset.
		inline OffsetRef STORM_FN operator +(Offset o) const {
			return OffsetRef(to, delta + o);
		}
		inline OffsetRef STORM_FN operator -(Offset o) const {
			return OffsetRef(to, delta - o);
		}

		// Get the offset.
		Offset STORM_FN offset() const;

		// Get the source.
		inline MAYBE(OffsetSource *) STORM_FN source() const { return to; }

		// Get the current difference from the base offset.
		inline Offset STORM_FN diff() const { return delta; }

	private:
		// Referring to.
		MAYBE(OffsetSource *) to;

		// Our offset.
		Offset delta;
	};

	wostream &operator <<(wostream &to, const OffsetRef &r);
	StrBuf &STORM_FN operator <<(StrBuf &to, OffsetRef r) ON(Compiler);

	/**
	 * Robust reference.
	 */
	class OffsetReference : public ObjectOn<Compiler> {
		STORM_CLASS;
		friend class OffsetRef;
		friend class OffsetSource;
		friend class Operand;
	public:
		// First parameter is what the reference should refer to, second is who is referring.
		STORM_CTOR OffsetReference(OffsetSource *to, Content *inside);
		STORM_CTOR OffsetReference(OffsetSource *to, Offset offset, Content *inside);
		STORM_CTOR OffsetReference(OffsetRef to, Content *inside);

		// Get the content we're associated with, ie. who's referring to something.
		inline Content *referrer() const { return owner; }

		// Notification of changed offset.
		virtual void moved(Offset newOffset);

		// Get current offset.
		Offset STORM_FN offset() const;

		// Get the source.
		inline MAYBE(OffsetSource *) STORM_FN source() const { return to; }

		// ToS.
		virtual void STORM_FN toS(StrBuf *to) const;

	private:
		// Owner = referrer
		Content *owner;

		// Who are we referring to?
		MAYBE(OffsetSource *) to;

		// Offset related to 'to'.
		Offset delta;

		// Called by OffsetSource.
		void onMoved(Offset newOffset);
	};

}
