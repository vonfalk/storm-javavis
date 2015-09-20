#include "stdafx.h"
#include "RefHandle.h"
#include "Shared/Str.h"

namespace storm {

	RefHandle::RefHandle(const code::Content &content) :
		isValue(false),
		destroyUpdater(null),
		createUpdater(null),
		deepCopyUpdater(null),
		equalsUpdater(null),
		hashUpdater(null),
		toSUpdater(null),
		content(content) {}

	RefHandle::~RefHandle() {
		delete destroyUpdater;
		delete createUpdater;
		delete deepCopyUpdater;
		delete equalsUpdater;
		delete hashUpdater;
		delete toSUpdater;
	}

	void RefHandle::output(const void *obj, StrBuf *to) const {
		if (!toS) {
			to->add(L"?");
			return;
		}

		Auto<Str> v;
		if (isValue) {
			// We can pass a direct pointer here.
			v = (*toS)(obj);
		} else {
			// We need to de-reference one time.
			const void **o = (const void **)obj;
			v = (*toS)(*o);
		}
		steal(to->add(v));
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

	void RefHandle::toSRef() {
		del(toSUpdater);
		hash = null;
	}

	void RefHandle::toSRef(const code::Ref &ref) {
		if (toSUpdater)
			toSUpdater->set(ref);
		else
			toSUpdater = new code::AddrReference((void **)&toS, ref, content);
	}

}
