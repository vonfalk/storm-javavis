#pragma once

#include "CriticalSection.h"

namespace util {

	//Used for posting results between threads.

	template <class R>
	class ResultWaiter {
	public:
		ResultWaiter();

		void set(const R &r);
		const R &get();
	private:
		bool hasResult;
		R result;
		CriticalSection s;
	};

	template <class R>
	ResultWaiter<R>::ResultWaiter() : hasResult(false) {}

	template <class R>
	void ResultWaiter<R>::set(const R &r) {
		CriticalSection::Section s(this->s);

		hasResult = true;
		result = r;
	}

	template <class R>
	const R &ResultWaiter<R>::get() {
		while (!hasResult) Sleep(1);

		CriticalSection::Section s(this->s);
		return result;
	}
}