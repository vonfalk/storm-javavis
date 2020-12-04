#include "stdafx.h"
#include "GraphicsWorkaround.h"
#include "OS/StackCall.h"

namespace gui {

	WorkaroundGraphics::WorkaroundGraphics(RenderInfo info, Painter *owner)
		: WindowGraphics(info, owner) {

		// 128 K should be enough.
		stack = new os::Stack(128 * 1024);
	}

	WorkaroundGraphics::~WorkaroundGraphics() {
		delete stack;
	}

	void WorkaroundGraphics::beforeRender() {
		os::stackCall(*stack, [this](){
								  WindowGraphics::beforeRender();
							  });
	}

	void WorkaroundGraphics::afterRender() {
		os::stackCall(*stack, [this](){
								  WindowGraphics::afterRender();
							  });
	}

	void WorkaroundGraphics::reset() {
		os::stackCall(*stack, [this](){
								  WindowGraphics::reset();
							  });
	}

	void WorkaroundGraphics::push() {
		os::stackCall(*stack, [this](){
								  WindowGraphics::push();
							  });
	}

	void WorkaroundGraphics::push(Float opacity) {
		os::stackCall(*stack, [this, opacity](){
								  WindowGraphics::push(opacity);
							  });
	}

	void WorkaroundGraphics::push(Rect clip) {
		os::stackCall(*stack, [this, &clip](){
								  WindowGraphics::push(clip);
							  });
	}

	void WorkaroundGraphics::push(Rect clip, Float opacity) {
		os::stackCall(*stack, [this, &clip, opacity](){
								  WindowGraphics::push(clip, opacity);
							  });
	}

	Bool WorkaroundGraphics::pop() {
		Bool ok = false;
		os::stackCall(*stack, [this, &ok](){
								  ok = WindowGraphics::pop();
							  });
		return ok;
	}

	void WorkaroundGraphics::line(Point from, Point to, Brush *brush) {
		os::stackCall(*stack, [this, &from, &to, brush](){
								  WindowGraphics::line(from, to, brush);
							  });
	}

	void WorkaroundGraphics::draw(Rect rect, Brush *brush) {
		os::stackCall(*stack, [this, &rect, brush](){
								  WindowGraphics::draw(rect, brush);
							  });
	}

	void WorkaroundGraphics::draw(Rect rect, Size edges, Brush *brush) {
		os::stackCall(*stack, [this, &rect, &edges, brush](){
								  WindowGraphics::draw(rect, edges, brush);
							  });
	}

	void WorkaroundGraphics::oval(Rect rect, Brush *brush) {
		os::stackCall(*stack, [this, &rect, brush](){
								  WindowGraphics::oval(rect, brush);
							  });
	}

	void WorkaroundGraphics::draw(Path *path, Brush *brush) {
		os::stackCall(*stack, [this, path, brush](){
								  WindowGraphics::draw(path, brush);
							  });
	}

	void WorkaroundGraphics::fill(Rect rect, Brush *brush) {
		os::stackCall(*stack, [this, &rect, brush](){
								  WindowGraphics::fill(rect, brush);
							  });
	}

	void WorkaroundGraphics::fill(Rect rect, Size edges, Brush *brush) {
		os::stackCall(*stack, [this, &rect, &edges, brush](){
								  WindowGraphics::fill(rect, edges, brush);
							  });
	}

	void WorkaroundGraphics::fill(Brush *brush) {
		os::stackCall(*stack, [this, brush](){
								  WindowGraphics::fill(brush);
							  });
	}

	void WorkaroundGraphics::fill(Path *path, Brush *brush) {
		os::stackCall(*stack, [this, path, brush](){
								  WindowGraphics::fill(path, brush);
							  });
	}

	void WorkaroundGraphics::fillOval(Rect rect, Brush *brush) {
		os::stackCall(*stack, [this, &rect, brush](){
								  WindowGraphics::fillOval(rect, brush);
							  });
	}

	void WorkaroundGraphics::draw(Bitmap *bitmap, Rect rect, Float opacity) {
		os::stackCall(*stack, [this, bitmap, &rect, opacity](){
								  WindowGraphics::draw(bitmap, rect, opacity);
							  });
	}

	void WorkaroundGraphics::draw(Bitmap *bitmap, Rect src, Rect dest, Float opacity) {
		os::stackCall(*stack, [this, bitmap, &src, &dest, opacity](){
								  WindowGraphics::draw(bitmap, src, dest, opacity);
							  });
	}

	void WorkaroundGraphics::text(Str *text, Font *font, Brush *brush, Rect rect) {
		os::stackCall(*stack, [this, text, font, brush, &rect](){
								  WindowGraphics::text(text, font, brush, rect);
							  });
	}

	void WorkaroundGraphics::draw(Text *text, Brush *brush, Point origin) {
		os::stackCall(*stack, [this, text, brush, &origin](){
								  WindowGraphics::draw(text, brush, origin);
							  });
	}

}
