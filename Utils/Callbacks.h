#pragma once

#include "Function.h"

#include <list>

namespace util {

	// This is a collection of callback functions. This class
	// will manage addition and removal of callbacks through
	// a convenient and standardized interface.
	template <class Parameter>
	class Callbacks : NoCopy {
	public:
		~Callbacks();

		// add a callback
		void add(const Function<void, Parameter> &fn) { cbList.push_back(fn); }

		// remove any callback comparing equal to "fn"
		void remove(const Function<void, Parameter> &fn);

		// call the callbacks
		void operator ()(Parameter p) const;
	private:
		typedef std::list<Function<void, Parameter> > CbList;
		CbList cbList;
	};

	template <class Parameter>
	Callbacks<Parameter>::~Callbacks() {}

	template <class Parameter>
	void Callbacks<Parameter>::remove(const util::Function<void, Parameter> &fn) {
		CbList::iterator i = cbList.begin();
		while (i != cbList.end()) {
			Function<void, Parameter> f = *i;
			if (f == fn) {
				i = cbList.erase(i);
			} else {
				++i;
			}
		}
	}

	template <class Parameter>
	void Callbacks<Parameter>::operator ()(Parameter p) const {
		for (CbList::const_iterator i = cbList.begin(); i != cbList.end(); i++) {
			Function<void, Parameter> fn = *i;
			fn(p);
		}
	}
}
