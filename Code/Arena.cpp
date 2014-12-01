#include "StdAfx.h"
#include "Arena.h"

namespace code {

	Arena::Arena() : addRef(null), releaseRef(null) {}

	Arena::~Arena() {
		assert(("Memory leak detected!", alloc.empty()));
		clear(externalRefs);
	}

	void *Arena::codeAlloc(nat size) {
		return alloc.allocate(size);
	}

	void Arena::codeFree(void *ptr) {
		if (ptr != null)
			alloc.free(ptr);
	}

	Ref Arena::external(const String &name, void *ptr) {
		RefSource *r = new RefSource(*this, name);
		r->set(ptr);
		externalRefs.push_back(r);
		return Ref(*r);
	}

	void Arena::preShutdown() {
		refManager.preShutdown();
	}

}
