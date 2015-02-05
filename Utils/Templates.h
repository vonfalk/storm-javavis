#pragma once

namespace templates {

	template <class T>
	class Identity {
	public:
		inline T &operator()(T &a) { return a; }
	};

}


// Detect if T is a pointer type.
template <class T>
struct TypeInfo {
	static inline bool reference() { return false; }
	static inline bool pointer() { return false; }
};

template <class T>
struct TypeInfo<T *> {
	static inline bool reference() { return false; }
	static inline bool pointer() { return true; }
};

template <class T>
struct TypeInfo<T &> {
	static inline bool reference() { return true; }
	static inline bool pointer() { return false; }
};


// Yes and no type.
struct YesType { byte z[1]; };
struct NoType { byte z[2]; };

// Can we convert From to To?
template <class From, class To>
struct IsConvertible {
	static NoType check(...);
	static YesType check(const To &);
	static From &from;
	enum { value = sizeof(check(from)) == sizeof(YesType) };
};

// Enable something if:
template <bool Cond, class T>
struct EnableIf {};

template <class T>
struct EnableIf<true, T> { typedef T t; };
