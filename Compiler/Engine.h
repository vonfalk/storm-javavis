#pragma once

#include "Gc.h"

// TODO: Do not depend on path!
#include "Utils/Path.h"

namespace storm {

	/**
	 * Defines the root object of the compiler. This object contains everything needed by the
	 * compiler itself, and shall be kept alive as long as anything from the compiler is used. Two
	 * separate instances of this class can be used as two completely separate runtime environments.
	 */
	class Engine : NoCopy {
	public:
		// Threading mode for the engine.
		enum ThreadMode {
			// Create a new main thread for the compiler (this is the storm::Compiler thread) to use
			// for compiling code. This means that the compiler will manage itself, but care must be
			// taken not to manipulate anything below the engine in another thread.
			newMain,

			// Reuses the caller of the constructor as the compiler thread (storm::Compiler). This
			// makes it easier to interface with the compiler itself, since all calls can be made
			// directly into the compiler, but care must be taken to allow the compiler to process
			// any messages sent to the compiler by calling UThread::leave(), or make the thread
			// wait in a os::Lock or os::Sema for events.
			reuseMain,
		};

		// Create the engine.
		// 'root' is the location of the root package directory on disk. The package 'core' is
		// assumed to be found as a subdirectory of the given root path.
		// TODO: Do not depend on Util/Path!
		Engine(const Path &root, ThreadMode mode);

		// Destroy. This will wait until all threads have terminated properly.
		~Engine();

	private:
		// Interface to the GC we're using.
		Gc gc;
	};

}
