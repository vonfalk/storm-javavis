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
			device = Device::create(engine());
		} catch (const storm::Exception *e) {
			PLN("Error while initializing rendering: " << e);
			throw;
		} catch (const ::Exception &e) {
			PLN("Error while initializing rendering: " << e);
			throw;
		}

		idMgr = new IdMgr();
	}

	Nat RenderMgr::allocId() {
		if (idMgr)
			return idMgr->alloc();
		else
			return 0;
	}

	void RenderMgr::freeId(Nat id) {
		// This is to ensure that we don't crash during shutdown.
		if (idMgr)
			idMgr->free(id);
	}

	void RenderMgr::attach(Resource *resource) {
		resources->put(resource);
	}

	Surface *RenderMgr::attach(Painter *painter, Handle window) {
		Surface *s = device->createSurface(window);
		if (s)
			painters->put(painter);
		return s;
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

		// Destroy all resources in all painters.
		for (Set<Painter *>::Iter i = painters->begin(), e = painters->end(); i != e; ++i) {
			i.v()->destroy();
			i.v()->destroyResources();
		}

		WeakSet<Resource>::Iter r = resources->iter();
		while (Resource *n = r.next())
			n->destroy();

		delete device;
		device = null;

		delete idMgr;
		idMgr = null;
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

	os::Thread spawnRenderThread(Engine &e) {
		struct Wrap {
			Engine &e;
			Wrap(Engine &e) : e(e) {}

			void run() {
				RenderMgr *m = gui::renderMgr(e);
				m->main();
				delete this;
			}
		};

		Wrap *wrap = new Wrap(e);
		util::Fn<void, void> fn(wrap, &Wrap::run);
		return os::Thread::spawn(fn, runtime::threadGroup(e));
	}

}
