#include "stdafx.h"
#include "Handle.h"

namespace storm {

	RefHandle::RefHandle() : destroyUpdater(null), createUpdater(null), deepCopyUpdater(null) {}

	RefHandle::~RefHandle() {
		delete destroyUpdater;
		delete createUpdater;
		delete deepCopyUpdater;
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

	void RefHandle::deepCopyRef(const code::Ref &ref) {
		if (deepCopyUpdater)
			deepCopyUpdater->set(ref);
		else
			deepCopyUpdater = new code::AddrReference((void **)&deepCopy, ref, L"handle-deep-copy");
	}

	void RefHandle::deepCopyRef() {
		del(deepCopyUpdater);
		deepCopy = null;
	}

	void RefHandle::createRef(const code::Ref &ref) {
		if (createUpdater)
			createUpdater->set(ref);
		else
			createUpdater = new code::AddrReference((void **)&create, ref, L"handle-create");
	}

}
