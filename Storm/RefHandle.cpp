#include "stdafx.h"
#include "RefHandle.h"

namespace storm {

	RefHandle::RefHandle(const code::Content &content) :
		destroyUpdater(null),
		createUpdater(null),
		deepCopyUpdater(null),
		equalsUpdater(null),
		hashUpdater(null),
		content(content) {}

	RefHandle::~RefHandle() {
		delete destroyUpdater;
		delete createUpdater;
		delete deepCopyUpdater;
		delete equalsUpdater;
		delete hashUpdater;
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

	void RefHandle::equalsRef() {
		del(equalsUpdater);
		equals = null;
	}

	void RefHandle::equalsRef(const code::Ref &ref) {
		if (equalsUpdater)
			equalsUpdater->set(ref);
		else
			equalsUpdater = new code::AddrReference((void **)&equals, ref, content);
	}

	void RefHandle::hashRef() {
		del(hashUpdater);
		hash = null;
	}

	void RefHandle::hashRef(const code::Ref &ref) {
		if (hashUpdater)
			hashUpdater->set(ref);
		else
			hashUpdater = new code::AddrReference((void **)&hash, ref, content);
	}

}
