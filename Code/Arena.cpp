#include "StdAfx.h"
#include "Arena.h"

namespace code {

	// Important to let 'refManager' initialize before 'addRef'.
	Arena::Arena() : refManager(), addRef(*this, L"addRef"), releaseRef(*this, L"releaseRef") {}

	Arena::~Arena() {
		clear(externalRefs);
		addRef.clear();
		releaseRef.clear();
		refManager.clear();
		if (!alloc.empty())
			PLN("Memory leak in code allocations detected!");
	}

	void *Arena::codeAlloc(nat size) {
		return alloc.allocate(size);
	}

	void Arena::codeFree(void *ptr) {
		if (ptr)
			alloc.free(ptr);
	}

	Ref Arena::external(const String &name, void *ptr) {
		RefSource *r = new RefSource(*this, name);
		r->setPtr(ptr);
		externalRefs.push_back(r);
		return Ref(*r);
	}

	void Arena::preShutdown() {
		refManager.preShutdown();
	}

}
