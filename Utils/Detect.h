#pragma once

/**
 * Detect members in a class. From: http://en.wikibooks.org/wiki/More_C%2B%2B_Idioms/Member_Detector
 *
 * Usage: CREATE_DETECTOR(attribute)
 * detect_<attribute><T>::value -> true or false.
 */
#define CREATE_DETECTOR(X)						\
	template <class T>							\
	class detect_##X {							\
		struct Fallback { int X; };				\
		struct Derived : T, Fallback {};		\
												\
		template <class U, U> struct Check;		\
												\
		typedef char ArrayOfOne[1];				\
		typedef char ArrayOfTwo[2];				\
												\
		template <class U>												\
		static ArrayOfOne &func(Check<int Fallback::*, &U::X> *);		\
		template <class U>												\
		static ArrayOfTwo &func(...);									\
	public:																\
	typedef detect_##X type;											\
	enum { value = sizeof(func<Derived>(0)) == sizeof(ArrayOfTwo) };	\
};
