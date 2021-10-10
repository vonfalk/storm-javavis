#include "stdafx.h"
#include "Search.h"

namespace storm {

	template <class T>
	T bSearch(T from, T to, Fn<Bool, T> *predicate) {
		while (from < to) {
			T mid = (to - from) / 2 + from;

			if (predicate->call(mid)) {
				from = mid + 1;
			} else {
				to = mid;
			}
		}

		return from;
	}

	Int binarySearch(Int from, Int to, Fn<Bool, Int> *predicate) {
		return bSearch<Int>(from, to, predicate);
	}

	Nat binarySearch(Nat from, Nat to, Fn<Bool, Nat> *predicate) {
		return bSearch<Nat>(from, to, predicate);
	}

	Long binarySearch(Long from, Long to, Fn<Bool, Long> *predicate) {
		return bSearch<Long>(from, to, predicate);
	}

	Word binarySearch(Word from, Word to, Fn<Bool, Word> *predicate) {
		return bSearch<Word>(from, to, predicate);
	}

}
