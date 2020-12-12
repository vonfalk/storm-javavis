#include "stdafx.h"
#include "StackDevice.h"
#include "StackGraphics.h"
#include "OS/StackCall.h"

namespace gui {

	// 128 K stack should be enough.
	StackWorkaround::StackWorkaround(SurfaceWorkaround *prev) : SurfaceWorkaround(prev), stack(128 * 1024) {}

	StackWorkaround::~StackWorkaround() {}

	Surface *StackWorkaround::applyThis(Surface *to) {
		return new StackSurface(this, to);
	}


	StackSurface::StackSurface(StackWorkaround *owner, Surface *wrap)
		: Surface(wrap->size, wrap->scale), owner(owner), wrap(wrap) {}

	StackSurface::~StackSurface() {
		delete wrap;
	}

	static WindowGraphics *CODECALL createGraphics(Surface *s, Engine *e) {
		return s->createGraphics(*e);
	}

	WindowGraphics *StackSurface::createGraphics(Engine &e) {
		Engine *engine = &e;
		os::FnCall<WindowGraphics *, 2> call = os::fnCall().add(wrap).add(engine);
		WindowGraphics *g = owner->call(address(gui::createGraphics), call);
		return new (e) StackGraphics(owner, g);
	}

	static void CODECALL resize(Surface *s, Size size, Float scale) {
		s->resize(size, scale);
	}

	void StackSurface::resize(Size size, Float scale) {
		os::FnCall<void, 3> call = os::fnCall().add(wrap).add(size).add(scale);
		owner->call(address(gui::resize), call);

		this->size = wrap->size;
		this->scale = wrap->scale;
	}

	static Surface::PresentStatus CODECALL present(Surface *wrap, bool vsync) {
		return wrap->present(vsync);
	}

	Surface::PresentStatus StackSurface::present(bool waitForVSync) {
		os::FnCall<PresentStatus, 2> call = os::fnCall().add(wrap).add(waitForVSync);
		return owner->call(address(gui::present), call);
	}

	static void CODECALL repaint(Surface *wrap, RepaintParams *params) {
		return wrap->repaint(params);
	}

	void StackSurface::repaint(RepaintParams *params) {
		os::FnCall<void, 2> call = os::fnCall().add(wrap).add(params);
		owner->call(address(gui::repaint), call);
	}

}
