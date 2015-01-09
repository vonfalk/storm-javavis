#pragma once

/**
 * Copy a map from another, swapping keys and values.
 */
template <class F, class T>
void mapSwap(const F &from, T &to) {
	F::const_iterator e = from.end();
	for (F::const_iterator i = from.begin(); i != e; ++i) {
		to.insert(make_pair(i->second, i->first));
	}
}

