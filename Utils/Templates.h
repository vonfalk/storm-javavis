#pragma once

namespace templates {

	template <class T>
	class Identity {
	public:
		inline T &operator()(T &a) { return a; }
	};

}


// Yes and no type.
struct YesType { bool z[1]; };
struct NoType { bool z[2]; };

// Can we convert From to To?
template <class From, class To>
struct IsConvertible {
	static NoType check(...);
	static YesType check(const To &);
	static From &from;
	enum { value = sizeof(check(from)) == sizeof(YesType) };
};

// Same type?
template <class T, class U>
struct SameType {
	enum { value = false };
};

template <class T>
struct SameType<T, T> {
	enum { value = true };
};

// Is a reference?
template <class T>
struct IsReference {
	enum { value = false };
};

template <class T>
struct IsReference<T &> {
	enum { value = true };
};

// Is the type void?
template <class T>
struct IsVoid {
	enum { value = false };
};

template <>
struct IsVoid<void> {
	enum { value = true };
};

// Enable something if:
template <bool Cond, class T>
struct EnableIf {};

template <class T>
struct EnableIf<true, T> { typedef T t; };

// Is T a floating point value?
template <class T>
struct IsFloat {
	enum { value = false };
};

template <>
struct IsFloat<float> {
	enum { value = true };
};

template <>
struct IsFloat<double> {
	enum { value = true };
};
