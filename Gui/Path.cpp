#include "stdafx.h"
#include "Path.h"
#include "LibData.h"
#include "RenderMgr.h"
#include "Exception.h"

namespace gui {

	Path::Path() : g(null), started(false) {
		elements = new (this) Array<Element>();
	}

	Path::~Path() {
		::release(g);
	}

	void Path::destroy() {
		::release(g);
	}

	void Path::clear() {
		elements->clear();
		started = false;
		b = Rect();
		invalidate();
	}

	void Path::start(Point pt) {
		Element e(tStart);
		e.start()->pt = pt;
		elements->push(e);
		if (elements->count() == 1)
			b = Rect(pt, Size());
		else
			b = b.include(pt);

		started = true;
		invalidate();
	}

	void Path::close() {
		Element e(tClose);
		elements->push(e);
		started = false;
		invalidate();
	}

	void Path::line(Point to) {
		Element e(tLine);
		e.line()->to = to;
		elements->push(e);
		b = b.include(to);
		invalidate();
	}

	void Path::point(Point to) {
		if (!started)
			start(to);
		else
			line(to);
	}

	void Path::bezier(Point c1, Point to) {
		Element e(tBezier2);
		e.bezier2()->c1 = c1;
		e.bezier2()->to = to;
		elements->push(e);
		b = b.include(c1).include(to);
		invalidate();
	}

	void Path::bezier(Point c1, Point c2, Point to) {
		Element e(tBezier3);
		e.bezier3()->c1 = c1;
		e.bezier3()->c2 = c2;
		e.bezier3()->to = to;
		elements->push(e);
		b = b.include(c1).include(c2).include(to);
		invalidate();
	}

	void Path::invalidate() {
		::release(g);
	}

#ifdef GUI_WIN32
	ID2D1PathGeometry *Path::geometry() {
		if (!g)
			create();
		return g;
	}

	void Path::create() {
		::release(g);

		RenderMgr *mgr = renderMgr(engine());
		mgr->attach(this);
		HRESULT r;
		ID2D1GeometrySink *sink = null;

		r = mgr->d2d()->CreatePathGeometry(&g);

		if (SUCCEEDED(r)) {
			r = g->Open(&sink);
		}

		if (SUCCEEDED(r)) {
			sink->SetFillMode(D2D1_FILL_MODE_ALTERNATE);
			bool started = false;

			for (Nat i = 0; i < elements->count(); i++) {
				Element &e = elements->at(i);
				switch (e.t) {
				case tStart:
					if (started)
						sink->EndFigure(D2D1_FIGURE_END_OPEN);
					sink->BeginFigure(dx(e.start()->pt), D2D1_FIGURE_BEGIN_FILLED);
					started = true;
					break;
				case tClose:
					if (started)
						sink->EndFigure(D2D1_FIGURE_END_CLOSED);
					started = false;
					break;
				case tLine:
					if (started)
						sink->AddLine(dx(e.line()->to));
					break;
				case tBezier2:
					if (started) {
						D2D1_QUADRATIC_BEZIER_SEGMENT s = { dx(e.bezier2()->c1), dx(e.bezier2()->to) };
						sink->AddQuadraticBezier(s);
					}
					break;
				case tBezier3:
					if (started) {
						D2D1_BEZIER_SEGMENT s = { dx(e.bezier3()->c1), dx(e.bezier3()->c2), dx(e.bezier3()->to) };
						sink->AddBezier(s);
					}
					break;
				}
			}

			if (started)
				sink->EndFigure(D2D1_FIGURE_END_OPEN);

			r = sink->Close();
		}

		if (FAILED(r)) {
			::release(g);
			throw GuiError(L"Failed to create path geometry: " + ::toS(r));
		}
	}

#endif
#ifdef GUI_GTK

	void Path::draw(NVGcontext *c) {
		nvgBeginPath(c);

		bool started = false;
		Point current;
		for (Nat i = 0; i < elements->count(); i++) {
			Element &e = elements->at(i);
			switch (e.t) {
			case tStart:
				current = e.start()->pt;
				nvgMoveTo(c, current.x, current.y);
				started = true;
				break;
			case tClose:
				nvgClosePath(c);
				started = false;
				break;
			case tLine:
				if (started) {
					current = e.line()->to;
					nvgLineTo(c, current.x, current.y);
				}
				break;
			case tBezier2:
				if (started) {
					Bezier2 *b = e.bezier2();
					current = b->to;
					nvgQuadTo(c, b->c1.x, b->c1.y, current.x, current.y);
				}
				break;
			case tBezier3:
				if (started) {
					Bezier3 *b = e.bezier3();
					current = b->to;
					nvgBezierTo(c, b->c1.x, b->c1.y, b->c2.x, b->c2.y, current.x, current.y);
				}
				break;
			}
		}
	}

	void Path::create() {}

#endif

}
