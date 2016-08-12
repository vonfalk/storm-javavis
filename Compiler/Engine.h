#pragma once

#include "Gc.h"
#include "RootArray.h"
#include "TemplateList.h"
#include "BootStatus.h"

// TODO: Do not depend on path!
#include "Utils/Path.h"

namespace storm {

	class Thread;

	/**
	 * Defines the root object of the compiler. This object contains everything needed by the
	 * compiler itself, and shall be kept alive as long as anything from the compiler is used. Two
	 * separate instances of this class can be used as two completely separate runtime environments.
	 *
	 * TODO? Break this into two parts; one that contains stuff needed from external libraries, and
	 * one that is central to the compiler.
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

		// Interface to the GC we're using.
		Gc gc;

		// Get a C++ type by its id.
		Type *cppType(Nat id) const;

		// Get a C++ template by its id.
		TemplateList *cppTemplate(Nat id) const;

		// Get a C++ named thread by its id.
		Thread *cppThread(Nat id) const;

		// Current boot status.
		inline BootStatus boot() const { return bootStatus; }

		// Advance boot status.
		void advance(BootStatus to);

		// Run the function for all named objects in the cache.
		typedef void (*NamedFn)(Named *);
		void forNamed(NamedFn fn);

		// Get the one and only pointer handle.
		const Handle &ptrHandle();

		// The threadgroup which all threads spawned from here shall belong to.
		os::ThreadGroup threadGroup;

	private:
		// All C++ types.
		RootArray<Type> cppTypes;

		// All C++ templates.
		RootArray<TemplateList> cppTemplates;

		// All named threads declared in C++.
		RootArray<Thread> cppThreads;

		// How far along in the boot process?
		BootStatus bootStatus;

		/**
		 * GC:d objects.
		 */
		struct GcRoot {
			// The one and only pointer handle, along with its root.
			Handle *pHandle;
		};

		GcRoot o;

		// Root for GcRoot.
		Gc::Root *objRoot;
	};

}
