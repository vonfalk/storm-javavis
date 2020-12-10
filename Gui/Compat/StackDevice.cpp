#include "stdafx.h"
#include "StackDevice.h"
#include "StackGraphics.h"
#include "OS/StackCall.h"

namespace gui {

	// 128 K stack should be enough.
	StackDevice::StackDevice(Device *wrap) : wrap(wrap), stack(128 * 1024) {}

	StackDevice::~StackDevice() {
		delete wrap;
	}

	static Surface *CODECALL createSurface(Device *d, Handle window) {
		return d->createSurface(window);
	}

	Surface *StackDevice::createSurface(Handle window) {
		os::FnCall<Surface *, 2> c = os::fnCall().add(wrap).add(window);
		Surface *s = call(address(gui::createSurface), c);
		if (s)
			return new StackSurface(this, s);
		else
			return null;
	}

	static TextMgr *CODECALL createTextMgr(Device *d) {
		return d->createTextMgr();
	}

	TextMgr *StackDevice::createTextMgr() {
		os::FnCall<TextMgr *, 2> c = os::fnCall().add(wrap);
		TextMgr *m = call(address(gui::createTextMgr), c);
		// Note: We're not wrapping the text manager due to recursive entries onto our stack.
		return m;
	}


	StackSurface::StackSurface(StackDevice *owner, Surface *wrap)
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
