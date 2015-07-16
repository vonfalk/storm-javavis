#include "stdafx.h"
#include "Path.h"
#include "RenderMgr.h"
#include "Exception.h"

namespace stormgui {

	Path::Path() : g(null) {}

	Path::~Path() {
		::release(g);
	}

	ID2D1PathGeometry *Path::geometry() {
		if (!g)
			create();
		return g;
	}

	void Path::clear() {
		elements.clear();
		b = Rect();
		invalidate();
	}

	void Path::start(Point pt) {
		Element e = { tStart };
		e.start.pt = dx(pt);
		elements.push_back(e);
		if (elements.size() == 1)
			b = Rect(pt, Size());
		else
			b = b.include(pt);
		invalidate();
	}

	void Path::line(Point to) {
		Element e = { tLine };
		e.line.to = dx(to);
		elements.push_back(e);
		b = b.include(to);
		invalidate();
	}

	void Path::point(Point to) {
		if (elements.size() == 0)
			start(to);
		else
			line(to);
	}

	void Path::bezier(Point c1, Point to) {
		Element e = { tBezier2 };
		e.bezier2.c1 = dx(c1);
		e.bezier2.to = dx(to);
		elements.push_back(e);
		b = b.include(c1).include(to);
		invalidate();
	}

	void Path::bezier(Point c1, Point c2, Point to) {
		Element e = { tBezier3 };
		e.bezier3.c1 = dx(c1);
		e.bezier3.c2 = dx(c2);
		e.bezier3.to = dx(to);
		elements.push_back(e);
		b = b.include(c1).include(c2).include(to);
		invalidate();
	}

	void Path::invalidate() {
		::release(g);
	}

	void Path::create() {
		::release(g);

		Auto<RenderMgr> mgr = renderMgr(engine());
		HRESULT r;
		ID2D1GeometrySink *sink = null;

		r = mgr->d2d()->CreatePathGeometry(&g);

		if (SUCCEEDED(r)) {
			r = g->Open(&sink);
		}

		if (SUCCEEDED(r)) {
			sink->SetFillMode(D2D1_FILL_MODE_ALTERNATE);
			bool started = false;

			for (nat i = 0; i < elements.size(); i++) {
				const Element &e = elements[i];
				switch (e.t) {
				case tStart:
					if (started)
						sink->EndFigure(D2D1_FIGURE_END_CLOSED);
					sink->BeginFigure(e.start.pt, D2D1_FIGURE_BEGIN_FILLED);
					started = true;
					break;
				case tLine:
					if (started)
						sink->AddLine(e.line.to);
					break;
				case tBezier2:
					if (started) {
						D2D1_QUADRATIC_BEZIER_SEGMENT s = { e.bezier2.c1, e.bezier2.to };
						sink->AddQuadraticBezier(s);
					}
					break;
				case tBezier3:
					if (started) {
						D2D1_BEZIER_SEGMENT s = { e.bezier3.c1, e.bezier3.c2, e.bezier3.to };
						sink->AddBezier(s);
					}
					break;
				}
			}
			if (started)
				sink->EndFigure(D2D1_FIGURE_END_CLOSED);

			r = sink->Close();
		}

		if (FAILED(r)) {
			::release(g);
			throw GuiError(L"Failed to create path geometry: " + ::toS(r));
		}
	}

}
