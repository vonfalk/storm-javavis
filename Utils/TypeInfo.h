#pragma once

/**
 * Information about a type.
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
		signedNr, // ie int, int64 ...
		unsignedNr, // ie nat, nat64 ...
		floatNr, // float, double ...
		user, // user defined type
	};

	// Kind.
	Kind kind;

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
