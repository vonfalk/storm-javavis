#include "stdafx.h"
#include "RenderMgr.h"
#include "Exception.h"
#include "Painter.h"
#include "LibData.h"
#include "Resource.h"
#include "Core/Array.h"

namespace gui {

	RenderMgr::RenderMgr() : exiting(false) {
		painters = new (this) Set<Painter *>();
		resources = new (this) WeakSet<Resource>();
		waitEvent = new (this) Event();
		exitSema = new (this) Sema(0);

		try {
			device = new Device(engine());
		} catch (const storm::Exception *e) {
			PLN("Error while initializing rendering: " << e);
			throw;
		} catch (const ::Exception &e) {
			PLN("Error while initializing rendering: " << e);
			throw;
		}

		// Start the thread if we need to.
		if (deviceType() == dtRaw) {
			RenderMgr *me = this;
			os::FnCall<void, 1> params = os::fnCall().add(me);
			os::UThread::spawn(address(&RenderMgr::main), true, params);
		} else {
			// To ensure we don't stall when terminating.
			exitSema->up();
		}
	}

	void RenderMgr::attach(Resource *resource) {
		resources->put(resource);
	}

	RenderInfo RenderMgr::attach(Painter *painter, Handle window) {
		RenderInfo r = device->attach(window);
		painters->put(painter);
		return r;
	}

	void RenderMgr::resize(RenderInfo &info, Size size, Float scale) {
		if (size != info.size)
			device->resize(info, size);
		info.scale = scale;
	}

	void RenderMgr::detach(Painter *painter) {
		// Painter::destroy calls detach, which will mess up the loop in "terminate" unless we're careful here.
		if (!exiting)
			painters->remove(painter);
	}

	void RenderMgr::terminate() {
		exiting = true;

		waitEvent->set();
		exitSema->down();

		// Destroy all resources.
		for (Set<Painter *>::Iter i = painters->begin(), e = painters->end(); i != e; ++i) {
			i.v()->destroy();
			i.v()->destroyResources();
		}

		WeakSet<Resource>::Iter r = resources->iter();
		while (Resource *n = r.next())
			n->destroy();

		delete device;
		device = null;
	}

	void RenderMgr::main() {
		// Note: Not always called. See the constructor.
		Array<Painter *> *toRedraw = new (this) Array<Painter *>();

		while (!exiting) {
			// Empty the array, reuse the storage.
			for (Nat i = 0; i < toRedraw->count(); i++)
				toRedraw->at(i) = null;

			// Figure out which we need to redraw this frame. Copy them since others may modify the
			// hash set as soon as we do UThread::leave.
			Nat pos = 0;
			for (Set<Painter *>::Iter i = painters->begin(), e = painters->end(); i != e; ++i) {
				Painter *p = i.v();
				// TODO: Why 'p->ready()' here? That means threads will have to poke at the event whenever they are ready!
				if (p->continuous && p->ready()) {
					if (pos >= toRedraw->count())
						toRedraw->push(p);
					else
						toRedraw->at(pos++) = p;
				}
			}

			Bool any = false;
			for (Nat i = 0; i < toRedraw->count(); i++) {
				if (toRedraw->at(i) == null)
					continue;
				any = true;

				// TODO: We probably want to wait for VSync once only, and not block this thread while doing so.
				// Note: It seems from the documentation for 'IDXGISwapChain::Present' that it schedules a buffer
				// swap but does not block until the next call to 'Present'. If so, we do not have to worry.
				try {
					Painter *p = toRedraw->at(i);
					p->doRepaint();
					p->present(true);
				} catch (const storm::Exception *e) {
					PLN(L"Error while rendering:\n" << e);
				} catch (const ::Exception &e) {
					PLN(L"Error while rendering:\n" << e);
				} catch (...) {
					PLN(L"Unknown error while rendering.");
				}
				os::UThread::leave();
			}

			if (!any)
				waitEvent->wait();
			waitEvent->clear();
		}

		exitSema->up();
	}

	void RenderMgr::painterReady() {
		waitEvent->set();
	}

	RenderMgr *renderMgr(EnginePtr e) {
		RenderMgr *&r = gui::renderData(e.v);
		if (!r)
			r = new (e.v) RenderMgr();
		return r;
	}

}
