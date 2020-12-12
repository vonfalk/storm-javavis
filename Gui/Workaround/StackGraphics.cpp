#include "stdafx.h"
#include "StackGraphics.h"

namespace gui {

	StackGraphics::StackGraphics(StackWorkaround *device, WindowGraphics *wrap)
		: owner(device), wrap(wrap) {}

	StackGraphics::~StackGraphics() {
		// We don't do this on the proper thread. The owner might not be alive anymore.
		delete wrap;
	}

	static void CODECALL surfaceResized(WindowGraphics *g) {
		g->surfaceResized();
	}

	void StackGraphics::surfaceResized() {
		os::FnCall<void, 2> call = os::fnCall().add(wrap);
		owner->call(address(gui::surfaceResized), call);
	}

	static void CODECALL surfaceDestroyed(WindowGraphics *g) {
		g->surfaceDestroyed();
	}

	void StackGraphics::surfaceDestroyed() {
		os::FnCall<void, 2> call = os::fnCall().add(wrap);
		owner->call(address(gui::surfaceResized), call);
	}

	static void CODECALL beforeRender(WindowGraphics *g, Color bgColor) {
		g->beforeRender(bgColor);
	}

	void StackGraphics::beforeRender(Color bgColor) {
		os::FnCall<void, 2> call = os::fnCall().add(wrap).add(bgColor);
		owner->call(address(gui::beforeRender), call);
	}

	static bool CODECALL afterRender(WindowGraphics *g) {
		return g->afterRender();
	}

	bool StackGraphics::afterRender() {
		os::FnCall<bool, 2> call = os::fnCall().add(wrap);
		return owner->call(address(gui::afterRender), call);
	}

	static void CODECALL destroy(WindowGraphics *g) {
		g->destroy();
	}

	void StackGraphics::destroy() {
		os::FnCall<void, 2> call = os::fnCall().add(wrap);
		owner->call(address(gui::destroy), call);
		WindowGraphics::destroy();
	}

	static void CODECALL reset(WindowGraphics *g) {
		g->reset();
	}

	void StackGraphics::reset() {
		os::FnCall<void, 2> call = os::fnCall().add(wrap);
		owner->call(address(gui::reset), call);
	}

	static void CODECALL push(WindowGraphics *g) {
		g->push();
	}

	void StackGraphics::push() {
		os::FnCall<void, 2> call = os::fnCall().add(wrap);
		owner->call(address(gui::push), call);
	}

	static void CODECALL pushF(WindowGraphics *g, Float opacity) {
		g->push(opacity);
	}

	void StackGraphics::push(Float opacity) {
		os::FnCall<void, 2> call = os::fnCall().add(wrap).add(opacity);
		owner->call(address(gui::pushF), call);
	}

	static void CODECALL pushR(WindowGraphics *g, Rect clip) {
		g->push(clip);
	}

	void StackGraphics::push(Rect clip) {
		os::FnCall<void, 2> call = os::fnCall().add(wrap).add(clip);
		owner->call(address(gui::pushR), call);
	}

	static void CODECALL pushRF(WindowGraphics *g, Rect clip, Float opacity) {
		g->push(clip, opacity);
	}

	void StackGraphics::push(Rect clip, Float opacity) {
		os::FnCall<void, 3> call = os::fnCall().add(wrap).add(clip);
		owner->call(address(gui::pushRF), call);
	}

	static Bool CODECALL pop(WindowGraphics *g) {
		return g->pop();
	}

	Bool StackGraphics::pop() {
		os::FnCall<Bool, 2> call = os::fnCall().add(wrap);
		return owner->call(address(gui::pop), call);
	}

	static void transform(WindowGraphics *g, Transform *tfm) {
		g->transform(tfm);
	}

	void StackGraphics::transform(Transform *tfm) {
		os::FnCall<void, 2> call = os::fnCall().add(wrap).add(tfm);
		owner->call(address(gui::transform), call);
	}

	static void lineWidth(WindowGraphics *g, Float w) {
		g->lineWidth(w);
	}

	void StackGraphics::lineWidth(Float w) {
		os::FnCall<void, 2> call = os::fnCall().add(wrap).add(w);
		owner->call(address(gui::lineWidth), call);
	}

	static void line(WindowGraphics *g, Point from, Point to, Brush *brush) {
		g->line(from, to, brush);
	}

	void StackGraphics::line(Point from, Point to, Brush *brush) {
		os::FnCall<void, 4> call = os::fnCall().add(wrap).add(from).add(to).add(brush);
		owner->call(address(gui::line), call);
	}

	static void drawR(WindowGraphics *g, Rect rect, Brush *brush) {
		g->draw(rect, brush);
	}

	void StackGraphics::draw(Rect rect, Brush *brush) {
		os::FnCall<void, 3> call = os::fnCall().add(wrap).add(rect).add(brush);
		owner->call(address(gui::drawR), call);
	}

	static void drawRR(WindowGraphics *g, Rect rect, Size edges, Brush *brush) {
		g->draw(rect, edges, brush);
	}

	void StackGraphics::draw(Rect rect, Size edges, Brush *brush) {
		os::FnCall<void, 4> call = os::fnCall().add(wrap).add(rect).add(edges).add(brush);
		owner->call(address(gui::drawRR), call);
	}

	static void oval(WindowGraphics *g, Rect rect, Brush *brush) {
		g->oval(rect, brush);
	}

	void StackGraphics::oval(Rect rect, Brush *brush) {
		os::FnCall<void, 3> call = os::fnCall().add(wrap).add(rect).add(brush);
		owner->call(address(gui::oval), call);
	}

	static void drawP(WindowGraphics *g, Path *path, Brush *brush) {
		g->draw(path, brush);
	}

	void StackGraphics::draw(Path *path, Brush *brush) {
		os::FnCall<void, 3> call = os::fnCall().add(wrap).add(path).add(brush);
		owner->call(address(gui::drawP), call);
	}

	static void fillR(WindowGraphics *g, Rect rect, Brush *brush) {
		g->fill(rect, brush);
	}

	void StackGraphics::fill(Rect rect, Brush *brush) {
		os::FnCall<void, 3> call = os::fnCall().add(wrap).add(rect).add(brush);
		owner->call(address(gui::fillR), call);
	}

	static void fillRR(WindowGraphics *g, Rect rect, Size edges, Brush *brush) {
		g->fill(rect, edges, brush);
	}

	void StackGraphics::fill(Rect rect, Size edges, Brush *brush) {
		os::FnCall<void, 4> call = os::fnCall().add(wrap).add(rect).add(edges).add(brush);
		owner->call(address(gui::fillRR), call);
	}

	static void fill(WindowGraphics *g, Brush *brush) {
		g->fill(brush);
	}

	void StackGraphics::fill(Brush *brush) {
		os::FnCall<void, 2> call = os::fnCall().add(wrap).add(brush);
		owner->call(address(gui::fill), call);
	}

	static void fillP(WindowGraphics *g, Path *path, Brush *brush) {
		g->fill(path, brush);
	}

	void StackGraphics::fill(Path *path, Brush *brush) {
		os::FnCall<void, 3> call = os::fnCall().add(wrap).add(path).add(brush);
		owner->call(address(gui::fillP), call);
	}

	static void fillOval(WindowGraphics *g, Rect rect, Brush *brush) {
		g->fillOval(rect, brush);
	}

	void StackGraphics::fillOval(Rect rect, Brush *brush) {
		os::FnCall<void, 3> call = os::fnCall().add(wrap).add(rect).add(brush);
		owner->call(address(gui::fillOval), call);
	}

	static void bitmapR(WindowGraphics *g, Bitmap *bitmap, Rect rect, Float opacity) {
		g->draw(bitmap, rect, opacity);
	}

	void StackGraphics::draw(Bitmap *bitmap, Rect rect, Float opacity) {
		os::FnCall<void, 4> call = os::fnCall().add(wrap).add(bitmap).add(rect).add(opacity);
		owner->call(address(gui::bitmapR), call);
	}

	static void bitmapRR(WindowGraphics *g, Bitmap *bitmap, Rect src, Rect dest, Float opacity) {
		g->draw(bitmap, src, dest, opacity);
	}

	void StackGraphics::draw(Bitmap *bitmap, Rect src, Rect dest, Float opacity) {
		os::FnCall<void, 5> call = os::fnCall().add(wrap).add(bitmap).add(src).add(dest).add(opacity);
		owner->call(address(gui::bitmapRR), call);
	}

	static void text(WindowGraphics *g, Str *text, Font *font, Brush *brush, Rect rect) {
		g->text(text, font, brush, rect);
	}

	void StackGraphics::text(Str *text, Font *font, Brush *brush, Rect rect) {
		os::FnCall<void, 5> call = os::fnCall().add(wrap).add(text).add(font).add(brush).add(rect);
		owner->call(address(gui::text), call);
	}

	static void drawT(WindowGraphics *g, Text *text, Brush *brush, Point origin) {
		g->draw(text, brush, origin);
	}

	void StackGraphics::draw(Text *text, Brush *brush, Point origin) {
		os::FnCall<void, 4> call = os::fnCall().add(wrap).add(text).add(brush).add(origin);
		owner->call(address(gui::drawT), call);
	}

}
