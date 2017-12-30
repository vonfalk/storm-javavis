#include "stdafx.h"
#include "LibData.h"
#include "App.h"
#include "RenderMgr.h"

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
	using namespace gui;
	return runtime::allocArray<void *>(e, &pointerArrayType, gui::entries);
}

void destroyLibData(void *data) {
	using namespace gui;

	GcArray<void *> *d = (GcArray<void *> *)data;

	// The RenderMgr assumes that the windowing system handled by App is still initialized during termination.
	RenderMgr *render = (RenderMgr *)d->v[1];
	if (render)
		render->terminate();

	// Now, we can safely destroy the App object.
	App *app = (App *)d->v[0];
	if (app)
		app->terminate();

}
