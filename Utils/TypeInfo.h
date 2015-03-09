#pragma once

/**
 * Minimal type information required for returning a value.
 * This is supposed to stay constant through different releases of the
 * compiler, so that it can easily be filled in from machine code. The
 * type information should be enough to figure out how to pass the return
 * value of a function.
 */
struct BasicTypeInfo {
	// Size of the type.
	nat size;

	// Kind of type?
	enum Kind {
		nothing = 0, // void
		signedNr, // int, int64...
		unsignedNr, // nat, nat64...
		floatNr, // float, double
		ptr, // pointer or reference
		user, // user defined type
	};

	Kind kind;

	// Plain? (ie not a pointer nor a reference).
	inline bool plain() const {
		return kind != ptr;
	}
};

wostream &operator <<(wostream &to, BasicTypeInfo::Kind k);
wostream &operator <<(wostream &to, const BasicTypeInfo &t);

/**
 * Information about a type. Things like copy-ctors and/or destructors may be added here in the future.
 * Use typeInfo<T>() to get an instance of this.
 */
struct TypeInfo {
	// Size of the type.
	size_t size;

	// Size of the type, ignoring modifiers. Eg. Foo * -> sizeof(Foo)
	size_t baseSize;

	// Is it a reference?
	bool ref;

	// Pointer depth.
	nat ptrs;

	// Plain (ie not a pointer nor a reference).
	inline bool plain() const {
		return !ref && ptrs == 0;
	}

	// Kind of type?
	enum Kind {
		nothing = 0, // ie void
		signedNr, // ie int, int64 ...
		unsignedNr, // ie nat, nat64 ...
		floatNr, // float, double ...
		user, // user defined type
	};

	// Kind.
	Kind kind;

	// Convert to the basic type info.
	operator BasicTypeInfo();
};

// Output.
wostream &operator <<(wostream &to, TypeInfo::Kind k);
wostream &operator <<(wostream &to, const TypeInfo &t);


/**
 * Get information about T.
 */
template <class T>
struct SizeOf {
	enum {
		size = sizeof(T),
		baseSize = sizeof(T),
	};
};

template <>
struct SizeOf<void> {
	enum {
		size = 0,
		baseSize = 0,
	};
};

template <>
struct SizeOf<const void> {
	enum {
		size = 0,
		baseSize = 0,
	};
};

template <class T>
struct SizeOf<T *> {
	enum {
		size = sizeof(T *),
		baseSize = SizeOf<T>::baseSize,
	};
};

template <class T>
struct SizeOf<T &> {
	enum {
		size = sizeof(T *),
		baseSize = SizeOf<T>::baseSize,
	};
};


template <class T>
struct ModifierOf {
	enum {
		depth = 0,
		ref = 0,
	};
};

template <class T>
struct ModifierOf<T *> {
	enum {
		depth = ModifierOf<T>::depth + 1,
		ref = 0,
	};
};

template <class T>
struct ModifierOf<T &> {
	enum {
		depth = ModifierOf<T>::depth,
		ref = 1,
	};
};

template <class T>
struct KindOf {
	enum { v = TypeInfo::user };
};

template <class T>
struct KindOf<T *> {
	enum { v = KindOf<T>::v };
};

template <class T>
struct KindOf<T &> {
	enum { v = KindOf<T>::v };
};

template <>
struct KindOf<void> {
	enum { v = TypeInfo::nothing };
};

template <>
struct KindOf<int> {
	enum { v = TypeInfo::signedNr };
};

template <>
struct KindOf<int64> {
	enum { v = TypeInfo::signedNr };
};

template <>
struct KindOf<short> {
	enum { v = TypeInfo::signedNr };
};

template <>
struct KindOf<char> {
	enum { v = TypeInfo::signedNr };
};

template <>
struct KindOf<nat> {
	enum { v = TypeInfo::unsignedNr };
};

template <>
struct KindOf<nat64> {
	enum { v = TypeInfo::unsignedNr };
};

template <>
struct KindOf<unsigned short> {
	enum { v = TypeInfo::unsignedNr };
};

template <>
struct KindOf<byte> {
	enum { v = TypeInfo::unsignedNr };
};

template <>
struct KindOf<float> {
	enum { v = TypeInfo::floatNr };
};

template <>
struct KindOf<double> {
	enum { v = TypeInfo::floatNr };
};

// General case, user-defined types.
template <class T>
TypeInfo typeInfo() {
	TypeInfo t = {
		SizeOf<T>::size,
		SizeOf<T>::baseSize,
		ModifierOf<T>::ref == 1,
		ModifierOf<T>::depth,
		(TypeInfo::Kind)KindOf<T>::v,
	};
	return t;
}
