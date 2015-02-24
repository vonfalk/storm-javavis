#include "StdAfx.h"
#include "Arena.h"

namespace code {

	// Important to let 'refManager' initialize before 'addRef'.
	Arena::Arena() : refManager(), addRef(*this, L"addRef"), releaseRef(*this, L"releaseRef") {}

	Arena::~Arena() {
		if (!alloc.empty())
			PLN("Memory leak in code allocations detected!");
		// assert(alloc.empty(), "Memory leak detected!");
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
