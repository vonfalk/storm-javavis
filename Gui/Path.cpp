#include "stdafx.h"
#include "Path.h"
#include "LibData.h"
#include "RenderMgr.h"
#include "Exception.h"
#include "GraphicsMgr.h"

namespace gui {

	Path::Path() : started(false) {
		elements = new (this) Array<PathPoint>();
	}

	void Path::create(GraphicsMgrRaw *g, void *&result, Cleanup &cleanup) {
		g->create(this, result, cleanup);
	}

	void Path::update(GraphicsMgrRaw *g, void *resource) {
		g->update(this, resource);
	}

	void Path::clear() {
		elements->clear();
		started = false;
		b = Rect();
		recreate();
	}

	void Path::start(Point pt) {
		PathPoint e(tStart);
		e.start()->pt = pt;
		elements->push(e);
		if (elements->count() == 1)
			b = Rect(pt, Size());
		else
			b = b.include(pt);

		started = true;
		recreate();
	}

	void Path::close() {
		PathPoint e(tClose);
		elements->push(e);
		started = false;
		recreate();
	}

	void Path::line(Point to) {
		PathPoint e(tLine);
		e.line()->to = to;
		elements->push(e);
		b = b.include(to);
		recreate();
	}

	void Path::point(Point to) {
		if (!started)
			start(to);
		else
			line(to);
	}

	void Path::bezier(Point c1, Point to) {
		PathPoint e(tBezier2);
		e.bezier2()->c1 = c1;
		e.bezier2()->to = to;
		elements->push(e);
		b = b.include(c1).include(to);
		recreate();
	}

	void Path::bezier(Point c1, Point c2, Point to) {
		PathPoint e(tBezier3);
		e.bezier3()->c1 = c1;
		e.bezier3()->c2 = c2;
		e.bezier3()->to = to;
		elements->push(e);
		b = b.include(c1).include(c2).include(to);
		recreate();
	}

	Array<PathPoint> *Path::data() {
		return new (this) Array<PathPoint>(*elements);
	}

#ifdef GUI_GTK

	void Path::draw(cairo_t *c) {
		cairo_new_path(c);

		bool started = false;
		Point current;
		for (Nat i = 0; i < elements->count(); i++) {
			PathPoint &e = elements->at(i);
			switch (e.t) {
			case tStart:
				current = e.start()->pt;
				cairo_move_to(c, current.x, current.y);
				started = true;
				break;
			case tClose:
				cairo_close_path(c);
				started = false;
				break;
			case tLine:
				if (started) {
					current = e.line()->to;
					cairo_line_to(c, current.x, current.y);
				}
				break;
			case tBezier2:
				if (started) {
					Bezier2 *b = e.bezier2();
					Point c1 = current + (2.0f/3.0f)*(b->c1 - current);
					Point c2 = b->to + (2.0f/3.0f)*(b->c1 - b->to);
					current = b->to;
					cairo_curve_to(c, c1.x, c1.y, c2.x, c2.y, current.x, current.y);
				}
				break;
			case tBezier3:
				if (started) {
					Bezier3 *b = e.bezier3();
					current = b->to;
					cairo_curve_to(c, b->c1.x, b->c1.y, b->c2.x, b->c2.y, current.x, current.y);
				}
				break;
			}
		}
	}

#endif

}
