#include "stdafx.h"
#include "App.h"
#include "LibData.h"

namespace gui {

	App::App() {}

	void App::terminate() {
		appWait->terminate();
	}

	App *app(EnginePtr e) {
		App *&v = appData(e.v);
		if (!v)
			v = new (e.v) App();
		return v;
	}

	AppWait::AppWait(Engine &e) {
		App *app = gui::app(e); // Make sure there is an 'app' object.
		app->appWait = this;
		events = 0;
	}

	AppWait::~AppWait() {
		g_source_destroy(fdSource);
		g_source_unref(fdSource);
	}

	void AppWait::init() {
		done = false;
		// TODO? Pass 'standard' parameters from the command line somehow...
		gtk_init(NULL, NULL);

		// TODO: We probably want to make sure we do not accidentally call into Gtk when we're in
		// some kind of callback from Gtk, similarly to what is done on Windows.

		// Create a main loop and a context.
		context = g_main_context_default();

		// Create the source for file descriptors.
		fdSource = g_source_new(&fdFuncs, sizeof(FdData));
		g_source_attach(fdSource, context);
	}

	static short from_g(GIOCondition src) {
		short r = 0;
		if (src & G_IO_IN)
			r |= POLLIN;
		if (src & G_IO_OUT)
			r |= POLLOUT;
		if (src & G_IO_PRI)
			r |= POLLPRI;
		if (src & G_IO_ERR)
			r |= POLLERR;
		if (src & G_IO_HUP)
			r |= POLLHUP;
		if (src & G_IO_NVAL)
			r |= POLLNVAL;
		return r;
	}

	static GIOCondition to_g(short src) {
		GIOCondition r = GIOCondition(0);
		if (src & POLLIN)
			r = GIOCondition(r | G_IO_IN);
		if (src & POLLOUT)
			r = GIOCondition(r | G_IO_OUT);
		if (src & POLLPRI)
			r = GIOCondition(r | G_IO_PRI);
		if (src & POLLERR)
			r = GIOCondition(r | G_IO_ERR);
		if (src & POLLHUP)
			r = GIOCondition(r | G_IO_HUP);
		if (src & POLLNVAL)
			r = GIOCondition(r | G_IO_NVAL);
		return r;
	}

	GSourceFuncs AppWait::fdFuncs = {
		null, // prepare
		&AppWait::fdCheck, // check
		null, // dispatch
		null, // finalize
	};

	gboolean AppWait::fdCheck(GSource *source) {
		FdData *me = (FdData *)source;
		if (!me->fds)
			return FALSE;

		os::IOHandle::Desc &desc = *me->fds;
		bool any = false;

		for (size_t i = 1; i < desc.count; i++) {
			short result = from_g(g_source_query_unix_fd(source, me->tags[i]));
			desc.fds[i].revents = result;
			any |= result != 0;
		}

		if (any) {
			atomicWrite(*me->notify, 1);
			g_main_context_wakeup(me->context);
		}

		return FALSE;
	}

	void AppWait::doWait(os::IOHandle &io) {
		// Prepare waiting for the fds.
		FdData *fdData = (FdData *)fdSource;
		os::IOHandle::Desc desc = io.desc();
		tags.resize(desc.count); // Grow to fit.
		for (size_t i = 1; i < desc.count; i++)
			tags[i] = g_source_add_unix_fd(fdSource, desc.fds[i].fd, to_g(desc.fds[i].events));
		fdData->context = context;
		fdData->fds = &desc;
		fdData->tags = tags.data();
		fdData->notify = &events;

		// Wait until we get an event. Perform more than one iteration if we have time.
		while (atomicRead(events) == 0)
			g_main_context_iteration(context, TRUE);

		// Reset the 'events' variable here. If we reset it just before we enter the wile loop, we
		// might miss events from 'signal' if we are unlucky.
		atomicWrite(events, 0);

		// Remove the file descriptors again.
		for (size_t i = 1; i < tags.size(); i++)
			g_source_remove_unix_fd(fdSource, tags[i]);
		fdData->fds = null;
		fdData->tags = null;
		fdData->notify = null;
	}

	bool AppWait::wait(os::IOHandle &io) {
		doWait(io);
		return !done;
	}

	gboolean AppWait::onTimeout(gpointer appWait) {
		AppWait *me = (AppWait *)appWait;
		atomicWrite(me->events, 1);
		g_main_context_wakeup(me->context);
		return TRUE;
	}

	bool AppWait::wait(os::IOHandle &io, nat msTimeout) {
		// Create a timeout.
		GSource *timeout = g_timeout_source_new(msTimeout);
		g_source_set_callback(timeout, &onTimeout, this, NULL);
		g_source_attach(timeout, context);

		// Wait for things to happen.
		doWait(io);

		// Destroy the timeout.
		g_source_destroy(timeout);
		g_source_unref(timeout);

		return !done;
	}

	void AppWait::signal() {
		atomicWrite(events, 1);
		// Note: causes the next invocation of 'g_main_context_iteration' to return without blocking
		// if no thread is currently blocking inside 'g_main_context_iteration'. This makes it
		// behave like a Condition, which is what we want.
		g_main_context_wakeup(context);
	}

	void AppWait::work() {
		// Try to dispatch a maximum of 100 events. Return as no event is dispatched so that we wait
		// properly instead of just burning CPU cycles for no good.
		bool dispatched = true;
		for (nat i = 0; i < 100 && dispatched; i++) {
			dispatched &= g_main_context_iteration(context, FALSE) == TRUE;
		}
	}

	void AppWait::terminate() {
		done = true;
		signal();
	}

	os::Thread spawnUiThread(Engine &e) {
		return os::Thread::spawn(new AppWait(e), runtime::threadGroup(e));
	}

}
