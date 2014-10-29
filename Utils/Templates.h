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

