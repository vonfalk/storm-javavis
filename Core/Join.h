#pragma once

namespace storm {

	/**
	 * C++ helpers for joining containers in storm into a string. Works with all containers that
	 * provide iterators. Only uses the value. Keys are ignored.
	 */

	template <class T>
	void join(StrBuf *to, T *src, const wchar *space) {
		typename T::Iter begin = src->begin();
		typename T::Iter end = src->end();

		if (begin == end)
			return;

		*to << begin.v();
		++begin;

		for (; begin != end; ++begin) {
			*to << space << begin.v();
		}
	}


	/**
	 * Allow using *buf << join(a, b) << ...
	 */
	template <class T>
	struct Join {
		T *src;
		const wchar *space;
	};

	template <class T>
	Join<T> join(T *src, const wchar *space) {
		Join<T> r = { src, space };
		return r;
	}

	template <class T>
	StrBuf &operator <<(StrBuf &to, const Join<T> &src) {
		join(&to, src.src, src.space);
		return to;
	}

}
