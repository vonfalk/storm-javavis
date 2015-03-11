#include "stdafx.h"
#include "CloneEnv.h"

namespace storm {

	CloneEnv::CloneEnv() {}

	CloneEnv::~CloneEnv() {
		for (ObjMap::iterator i = objs.begin(), end = objs.end(); i != end; ++i) {
			i->first->release();
			i->second->release();
		}
	}

	Object *CloneEnv::cloned(Par<Object> o) {
		ObjMap::iterator i = objs.find(o.borrow());
		if (i != objs.end()) {
			i->second->addRef();
			return i->second;
		}
		return null;
	}

	void CloneEnv::cloned(Par<Object> o, Par<Object> to) {
		ObjMap::iterator i = objs.find(o.borrow());
		if (i != objs.end()) {
			i->second->release();
			i->second = to.borrow();
			i->second->addRef();
		} else {
			objs.insert(make_pair(o.borrow(), to.borrow()));
			o->addRef();
			to->addRef();
		}
	}

}
