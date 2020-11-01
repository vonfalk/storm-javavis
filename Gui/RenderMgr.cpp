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

#ifdef UI_MULTITHREAD
		waitEvent->set();
		exitSema->down();
#endif

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
		// Note: Not called if SINGLE_THREADED_UI is defined.
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
					toRedraw->at(i)->doRepaint(true, false);
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

#ifdef UI_SINGLETHREAD

	os::Thread spawnRenderThread(Engine &e) {
		// We use the Ui thread here, since we do not want true multithreaded rendering for some reason.
		return Ui::thread(e)->thread();
	}

#endif
#ifdef UI_MULTITHREAD

	os::Thread spawnRenderThread(Engine &e) {
		// Ugly hack...
		struct Wrap {
			void attach() {
				Engine &e = (Engine &)*this;
				RenderMgr *m = gui::renderMgr(e);
				m->main();
			}
		};

		util::Fn<void, void> fn((Wrap *)&e, &Wrap::attach);
		return os::Thread::spawn(fn, runtime::threadGroup(e));
	}

#endif

}
