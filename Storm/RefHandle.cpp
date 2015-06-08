#include "stdafx.h"
#include "RefHandle.h"

namespace storm {

	RefHandle::RefHandle(const code::Content &content) :
		destroyUpdater(null),
		createUpdater(null),
		deepCopyUpdater(null),
		content(content) {}

	RefHandle::~RefHandle() {
		delete destroyUpdater;
		delete createUpdater;
		delete deepCopyUpdater;
	}

	void RefHandle::destroyRef(const code::Ref &ref) {
		if (destroyUpdater)
			destroyUpdater->set(ref);
		else
			destroyUpdater = new code::AddrReference((void **)&destroy, ref, content);
	}

	void RefHandle::destroyRef() {
		del(destroyUpdater);
		destroy = null;
	}

	void RefHandle::deepCopyRef(const code::Ref &ref) {
		if (deepCopyUpdater)
			deepCopyUpdater->set(ref);
		else
			deepCopyUpdater = new code::AddrReference((void **)&deepCopy, ref, content);
	}

	void RefHandle::deepCopyRef() {
		del(deepCopyUpdater);
		deepCopy = null;
	}

	void RefHandle::createRef(const code::Ref &ref) {
		if (createUpdater)
			createUpdater->set(ref);
		else
			createUpdater = new code::AddrReference((void **)&create, ref, content);
	}

}
