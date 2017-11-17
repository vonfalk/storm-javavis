#pragma once
#include "OS/IOCondition.h"

namespace gui {

	class AppWait;

	class App : public ObjectOn<Ui> {
		STORM_CLASS;
	public:

		// Terminate all activity.
		void terminate();

	private:
		friend class AppWait;
		friend App *app(EnginePtr e);

		// Create the instance.
		App();

		// Remember the AppWait for this thread so that we can talk to it.
		AppWait *appWait;
	};

	// Get the App object.
	App *STORM_FN app(EnginePtr engine) ON(Ui);

	/**
	 * Custom wait object for the Ui thread.
	 */
	class AppWait : public os::ThreadWait {
	public:
		AppWait(Engine &e);
		~AppWait();

		virtual void init();
		virtual bool wait(os::IOHandle &io);
		virtual bool wait(os::IOHandle &io, nat msTimeout);
		virtual void signal();
		virtual void work();

		// Note that we shall shut down.
		void terminate();

	private:
		// Shall we exit?
		bool done;

		// Did any events trigger? Used with atomics.
		nat events;

		// The global main context.
		GMainContext *context;

		// The source we use to attach file descriptors to.
		GSource *fdSource;

		// Data for our fdSource.
		struct FdData {
			GSource super;
			GMainContext *context;
			os::IOHandle::Desc *fds;
			gpointer *tags;
			nat *notify;
		};

		// Function table for our GSource.
		static GSourceFuncs fdFuncs;

		// Callback from fdFuncs.
		static gboolean fdCheck(GSource *source);

		// Callback from GLib.
		static gboolean onTimeout(gpointer appWait);

		// Used in 'fdCheck'.
		vector<gpointer> tags;

		// Wait for a callback of some sort.
		void doWait(os::IOHandle &io);
	};

	// Spawn the UI thread.
	os::Thread spawnUiThread(Engine &e);

}
