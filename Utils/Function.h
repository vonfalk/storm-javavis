#pragma once

#include "Blob.h"

namespace fn {
	// Internal helpers for the function class.
	typedef Blob<2 * sizeof(void *)> PtrBlob; // Size is larger for complex fn ptrs sometimes

	// Do nothing
	template <class ReturnType, class ParameterType>
	ReturnType simpleNop(void *, const PtrBlob &ptr, ParameterType param) {
		return ReturnType();
	}

	template <class ReturnType>
	ReturnType simpleNopVoid(void *, const PtrBlob &ptr) {
		return ReturnType();
	}

	// Run a simple function pointer
	template <class ReturnType, class ParameterType>
	ReturnType simpleFnExecute(void *, const PtrBlob &ptr, ParameterType param) {
		typedef ReturnType (* FnPtr)(ParameterType);
		FnPtr fnPtr = ptr.get<FnPtr>();
		return (*fnPtr)(param);
	}

	template <class ReturnType>
	ReturnType simpleFnExecuteVoid(void *, const PtrBlob &ptr) {
		typedef ReturnType (* FnPtr)();
		FnPtr fnPtr = ptr.get<FnPtr>();
		return (*fnPtr)();
	}

	// Run a member function ptr
	template <class ReturnType, class ParameterType, class C>
	ReturnType memberFnExecute(void *thisPtr, const PtrBlob &ptr, ParameterType param) {
		typedef ReturnType (C:: *FnPtr)(ParameterType);
		FnPtr fnPtr = ptr.get<FnPtr>();
		C *cThisPtr = (C *)thisPtr;
		return (cThisPtr->*fnPtr)(param);
	}

	template <class ReturnType, class C>
	ReturnType memberFnExecuteVoid(void *thisPtr, const PtrBlob &ptr) {
		typedef ReturnType (C:: *FnPtr)();
		FnPtr fnPtr = ptr.get<FnPtr>();
		C *cThisPtr = (C *)thisPtr;
		return (cThisPtr->*fnPtr)();
	}
}

// Function pointer class. This class encapsulates a function pointer either to a simple C-style function
// or to a member function. If it contains a member function to a class, it will also contain the "this" pointer
// for that instance, so that functions encapsulated inside this object always can be treated as a simple C-style
// function.
// In the case of an empty function pointer, it will simply return ReturnType() for the specified return type.
template <class ReturnType, class ParameterType = void>
class Fn {
public:
	// Create an empty fn ptr
	Fn() {
		thisPtr = null;
		toExecute = &fn::simpleNop<ReturnType, ParameterType>;
	}

	// Create from a simple function pointer.
	Fn(ReturnType (*fnPtr)(ParameterType)) {
		thisPtr = null;
		this->fnPtr.set(fnPtr);
		toExecute = &fn::simpleFnExecute<ReturnType, ParameterType>;
	}

	// Create from a member function.
	template <class C>
	Fn(C *thisPtr, ReturnType (C::* fnPtr)(ParameterType)) {
		this->thisPtr = thisPtr;
		this->fnPtr.set(fnPtr);
		toExecute = &fn::memberFnExecute<ReturnType, ParameterType, C>;
	}

	inline ReturnType operator ()(ParameterType p) const {
		return (*toExecute)(thisPtr, fnPtr, p);
	}

	bool operator ==(const Fn<ReturnType, ParameterType> &other) const {
		if (thisPtr != other.thisPtr) return false;
		if (fnPtr != other.fnPtr) return false;
		if (toExecute != other.toExecute) return false;
		return true;
	}
	inline bool operator !=(const Fn<ReturnType, ParameterType> &other) const { return !(*this == other); }

private:
	void *thisPtr;
	fn::PtrBlob fnPtr;

	ReturnType (* toExecute)(void *, const fn::PtrBlob &, ParameterType);
};

// Specialization for <X, void>
template <class ReturnType>
class Fn<ReturnType, void> {
public:
	// Create an empty fn ptr
	Fn() {
		thisPtr = null;
		toExecute = &fn::simpleNopVoid<ReturnType>;
	}

	// Create from a simple function pointer.
	Fn(ReturnType (*fnPtr)()) {
		thisPtr = null;
		this->fnPtr.set(fnPtr);
		toExecute = &fn::simpleFnExecuteVoid<ReturnType>;
	}

	// Create from a member function.
	template <class C>
	Fn(C *thisPtr, ReturnType (C::* fnPtr)()) {
		this->thisPtr = thisPtr;
		this->fnPtr.set(fnPtr);
		toExecute = &fn::memberFnExecuteVoid<ReturnType, C>;
	}

	inline ReturnType operator ()() const {
		return (*toExecute)(thisPtr, fnPtr);
	}

	bool operator ==(const Fn<ReturnType, void> &other) const {
		if (thisPtr != other.thisPtr) return false;
		if (fnPtr != other.fnPtr) return false;
		if (toExecute != other.toExecute) return false;
		return true;
	}
	inline bool operator !=(const Fn<ReturnType, void> &other) const { return !(*this == other); }

private:
	void *thisPtr;
	fn::PtrBlob fnPtr;

	ReturnType (* toExecute)(void *, const fn::PtrBlob &);
};

// Create objects easily
template <class A, class B>
inline Fn<A, B> simpleFn(A (* p)(B)) {
	return Fn<A, B>(p);
}

template <class A>
inline Fn<A, void> simpleVoidFn(A (* p)()) {
	return Fn<A, void>(p);
}

template <class A, class B, class C>
inline Fn<A, B> memberFn(C *t, A (C:: *p)(B) const) {
	return Fn<A, B>(t, (A (C:: *)(B))p);
}

template <class A, class B, class C>
inline Fn<A, B> memberFn(C *t, A (C:: *p)(B)) {
	return Fn<A, B>(t, p);
}

template <class A, class C>
inline Fn<A, void> memberVoidFn(C *t, A (C:: *p)() const) {
	return Fn<A, void>(t, (A (C:: *)())p);
}

template <class A, class C>
inline Fn<A, void> memberVoidFn(C *t, A (C:: *p)()) {
	return Fn<A, void>(t, p);
}
