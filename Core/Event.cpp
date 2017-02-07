#include "stdafx.h"
#include "Event.h"

namespace storm {

	Event::Event() : alloc(new Data()) {}

	Event::Event(const Event &o) : alloc(o.alloc) {
		atomicIncrement(alloc->refs);
	}

	Event::~Event() {
		if (atomicDecrement(alloc->refs) == 0) {
			delete alloc;
		}
	}

	void Event::deepCopy(CloneEnv *) {
		// Nothing to do.
	}

	void Event::set() {
		alloc->event.set();
	}

	void Event::clear() {
		alloc->event.clear();
	}

	void Event::wait() {
		alloc->event.wait();
	}

	Bool Event::isSet() {
		return alloc->event.isSet();
	}

	Event::Data::Data() : refs(1) {}

}
