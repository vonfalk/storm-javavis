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

}
