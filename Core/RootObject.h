#pragma once
#include "Runtime.h"
#include "OS/PtrThrowable.h"

namespace storm {
	STORM_PKG(core.lang);

	class Engine;
	class Str;
	class StrBuf;
	struct RootCast;

	/**
	 * The shared parts between Object and TObject. Not exposed to Storm.
	 */
	class RootObject : public os::PtrThrowable {
		STORM_ROOT_CLASS;
	public:
		// Default constructor.
		RootObject();

		// Default copy-ctor.
		RootObject(const RootObject &o);

		// Make sure destructors are virtual.
		virtual ~RootObject();

		// Get the engine somehow.
		inline Engine &engine() const {
			return runtime::allocEngine(this);
		}

		// Get our type somehow.
		inline Type *type() const {
			return runtime::typeOf(this);
		}

		// Used to allow the as<Foo> using our custom (fast) type-checking.
		inline bool isA(const Type *o) const {
			return runtime::isA(this, o);
		}

		// Exception magic. More or less only used to produce nice error messages in some cases.
		virtual const wchar *toCStr() const;

		// To string.
		virtual Str *STORM_FN toS() const;
		virtual void STORM_FN toS(StrBuf *to) const;

		// Custom casting using as<>.
		typedef RootCast DynamicCast;
	};


	// Output an object.
	wostream &operator <<(wostream &to, const RootObject *o);
	wostream &operator <<(wostream &to, const RootObject &o);


	/**
	 * Custom casting.
	 */
	struct RootCast {
		template <class To>
		static To *cast(RootObject *from) {
			if (from == null)
				return null;
			if (from->isA(To::stormType(from->engine())))
				return static_cast<To *>(from);
			return null;
		}

		template <class To>
		static To *cast(const RootObject *from) {
			if (from == null)
				return null;
			if (from->isA(To::stormType(from->engine())))
				return static_cast<To *>(from);
			return null;
		}
	};


	/**
	 * Check if two objects have the same type. Useful in operator ==.
	 */
	inline Bool sameType(const RootObject *a, const RootObject *b) {
		return runtime::typeOf(a) == runtime::typeOf(b);
	}

}
