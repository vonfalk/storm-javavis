#pragma once
#include "Fn.h"

namespace storm {
	STORM_PKG(core);

	// Binary search. Searches the half-open interval [from,to[ and returns the first index for
	// which the supplied predicate returns false (this might be =to if the predicate is true for
	// all elements). The predicate is expected to act as a <-operator on the data (checking if the
	// supplied parameter is less than the value searched for). This means that the predicate is
	// expected to be monotone. These functions can be used to search in a sorted array with a
	// suitable lambda function.
	//
	// Example 1: This will simply find the number 42:
	// -> binarySearch(0, 1000, (x) => x < 42)
	//
	// Example 2: This will find the element in the array that is greater or equal to 50:
	// -> var x = [1, 10, 100, 1000];
	// -> binarySearch(0, x.count, (i) => x[i] < 50)
	Int STORM_FN binarySearch(Int from, Int to, Fn<Bool, Int> *predicate);
	Nat STORM_FN binarySearch(Nat from, Nat to, Fn<Bool, Nat> *predicate);
	Long STORM_FN binarySearch(Long from, Long to, Fn<Bool, Long> *predicate);
	Word STORM_FN binarySearch(Word from, Word to, Fn<Bool, Word> *predicate);

}
