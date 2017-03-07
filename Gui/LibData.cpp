#include "stdafx.h"
#include "LibData.h"
#include "App.h"

namespace gui {

	static const Nat entries = 2;

	static inline GcArray<void *> *data(Engine &e) {
		return (GcArray<void *> *)e.data();
	}

	App *&appData(Engine &e) {
		return (App *&)data(e)->v[0];
	}

	RenderMgr *&renderData(Engine &e) {
		return (RenderMgr *&)data(e)->v[1];
	}

}

void *createLibData(storm::Engine &e) {
	return runtime::allocArray<void *>(e, &pointerArrayType, gui::entries);
}

void destroyLibData(void *data) {
	using namespace gui;

	GcArray<void *> *d = (GcArray<void *> *)data;
	App *app = (App *)d->v[0];
	if (app)
		app->destroy();

	TODO(L"Clean up the renderer as well!");
}
