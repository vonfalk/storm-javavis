#pragma once
#include "Runtime.h"

namespace storm {
	STORM_PKG(core.lang);

	class Engine;
	class Str;
	class StrBuf;

	/**
	 * The shared parts between Object and TObject. Not exposed to Storm.
	 */
	class RootObject {
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

		// To string.
		virtual Str *STORM_FN toS() const;
		virtual void STORM_FN toS(StrBuf *to) const;
	};


	// Output an object.
	wostream &operator <<(wostream &to, const RootObject *o);
	wostream &operator <<(wostream &to, const RootObject &o);
}


/**
 * Custom casting.
 */
template <class Src>
struct RootCast {
	Src *from;
	RootCast(Src *from) : from(from) {}

	template <class To>
	To *cast() const {
		if (from == null)
			return null;
		if (from->isA(To::stormType(from->engine())))
			return static_cast<To *>(from);
		return null;
	}
};

inline RootCast<storm::RootObject> customCast(storm::RootObject *from) {
	TODO(L"Check so that these are properly used!");
	return RootCast<storm::RootObject>(from);
}

inline RootCast<const storm::RootObject> customCast(const storm::RootObject *from) {
	TODO(L"Check so that these are properly used!");
	return RootCast<const storm::RootObject>(from);
}
