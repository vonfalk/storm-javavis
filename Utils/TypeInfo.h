#pragma once
#include <type_traits>

#if defined(VISUAL_STUDIO) && VISUAL_STUDIO <= 2008

namespace std {
	template <class T>
	struct is_trivially_copy_constructible {
		static const bool value = std::tr1::has_trivial_copy<T>::value;
	};

	template <class T>
	struct is_trivially_destructible {
		static const bool value = std::tr1::has_trivial_destructor<T>::value;
	};

	template <class T>
	struct is_pod {
		static const bool value = std::tr1::is_pod<T>::value;
	};
}

#endif


struct TypeKind {
	// Kind of type?
	enum T {
		nothing = 0, // void
		signedNr, // int, int64...
		unsignedNr, // nat, nat64...
		floatNr, // float, double
		boolVal, // bool
		ptr, // pointer or reference
		userTrivial, // user defined type, trivial copy ctor
		userComplex, // user defined type, nontrivial copy ctor (or destructor)
	};
};

wostream &operator <<(wostream &to, TypeKind::T t);

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

	typedef TypeKind::T Kind;

	// Kind of type?
	Kind kind;

	// Plain? (ie not a pointer nor a reference).
	inline bool plain() const {
		return kind != TypeKind::ptr;
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
	typedef TypeKind::T Kind;

	// Kind. Note: 'ptr' is never used.
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

template <bool trivial>
struct TrivialKind {
	enum { v = TypeKind::userComplex };
};

template <>
struct TrivialKind<true> {
	enum { v = TypeKind::userTrivial };
};

template <class T>
struct KindOf {
	enum { v = TrivialKind<
		   std::is_trivially_copy_constructible<T>::value
		   & std::is_trivially_destructible<T>::value
		   >::v };
};

template <class T>
struct KindOf<T *> {
	enum { v = KindOf<T>::v };
};

template <class T>
struct KindOf<T &> {
	enum { v = KindOf<T>::v };
};

template <class T>
struct KindOf<const T> {
	enum { v = KindOf<T>::v };
};

template <>
struct KindOf<void> {
	enum { v = TypeKind::nothing };
};

template <>
struct KindOf<int> {
	enum { v = TypeKind::signedNr };
};

template <>
struct KindOf<int64> {
	enum { v = TypeKind::signedNr };
};

template <>
struct KindOf<short> {
	enum { v = TypeKind::signedNr };
};

template <>
struct KindOf<char> {
	enum { v = TypeKind::signedNr };
};

template <>
struct KindOf<nat> {
	enum { v = TypeKind::unsignedNr };
};

template <>
struct KindOf<nat64> {
	enum { v = TypeKind::unsignedNr };
};

template <>
struct KindOf<unsigned short> {
	enum { v = TypeKind::unsignedNr };
};

template <>
struct KindOf<byte> {
	enum { v = TypeKind::unsignedNr };
};

template <>
struct KindOf<bool> {
	enum { v = TypeKind::boolVal };
};

template <>
struct KindOf<float> {
	enum { v = TypeKind::floatNr };
};

template <>
struct KindOf<double> {
	enum { v = TypeKind::floatNr };
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
