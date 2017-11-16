#include "stdafx.h"
#include "LibData.h"
#include "App.h"

namespace gui {

	static const Nat entries = 1;

	static inline GcArray<void *> *data(Engine &e) {
		return (GcArray<void *> *)e.data();
	}

	App *&appData(Engine &e) {
		return (App *&)data(e)->v[0];
	}

}

void *createLibData(storm::Engine &e) {
	using namespace gui;
	return runtime::allocArray<void *>(e, &pointerArrayType, entries);
}

void destroyLibData(void *data) {
	using namespace gui;
	GcArray<void *> *d = (GcArray<void *> *)data;
	App *app = (App *)d->v[0];
	if (app)
		app->terminate();
}
