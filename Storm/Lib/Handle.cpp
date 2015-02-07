#include "stdafx.h"
#include "Handle.h"

namespace storm {

	RefHandle::RefHandle() : destroyUpdater(null), createUpdater(null) {}

	RefHandle::~RefHandle() {
		delete destroyUpdater;
		delete createUpdater;
	}

	void RefHandle::destroyRef(const code::Ref &ref) {
		if (destroyUpdater)
			destroyUpdater->set(ref);
		else
			destroyUpdater = new code::AddrReference((void **)&destroy, ref, L"handle-destroy");
	}

	void RefHandle::destroyRef() {
		del(destroyUpdater);
		destroy = null;
	}

	void RefHandle::createRef(const code::Ref &ref) {
		if (createUpdater)
			createUpdater->set(ref);
		else
			createUpdater = new code::AddrReference((void **)&create, ref, L"handle-create");
	}

}
