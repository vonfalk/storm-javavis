#pragma once

namespace gui {

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
	};

	// Get the App object.
	App *STORM_FN app(EnginePtr engine) ON(Ui);

	// Spawn the UI thread.
	os::Thread spawnUiThread(Engine &e);

}
