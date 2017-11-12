#include "stdafx.h"
#include "Data.h"
#include "Core/Runtime.h"

namespace testlib {

	static const Nat entries = 1;

	static inline GcArray<void *> *data(Engine &e) {
		return (GcArray<void *> *)e.data();
	}

	GlobalData *&globalData(Engine &e) {
		return (GlobalData *&)data(e)->v[0];
	}

	void GlobalData::toS(StrBuf *to) const {
		*to << counter;
	}

	GlobalData *global(EnginePtr e) {
		GlobalData *&v = globalData(e.v);
		if (!v)
			v = new (e.v) GlobalData();
		v->counter++;
		return v;
	}

}

void *createLibData(storm::Engine &e) {
	return storm::runtime::allocArray<void *>(e, &storm::pointerArrayType, testlib::entries);
}

void destroyLibData(void *data) {
	// We do not need to do anything special here.
}
