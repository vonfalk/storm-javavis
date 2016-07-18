#include "stdafx.h"
#include "Engine.h"
#include "Init.h"
#include "Thread.h"
#include "Type.h"

// Only included from here:
#include "OS/SharedMaster.h"

namespace storm {

	// Default arena size. 32 MB should be enough for a while at least.
	static const size_t defaultArena = 32 * 1024 * 1024;

	// Get the compiler thread from flags.
	static os::Thread mainThread(Engine::ThreadMode mode, Engine &e) {
		switch (mode) {
		case Engine::newMain:
			return os::Thread::spawn(Thread::registerFn(e), e.threadGroup);
		case Engine::reuseMain:
			Thread::registerFn(e)();
			return os::Thread::current();
		default:
			assert(false, L"Unknown thread mode.");
			WARNING(L"Unknown thread mode, defaulting to 'current'.");
			return os::Thread::current();
		}
	}

	Engine::Engine(const Path &root, ThreadMode mode) : gc(defaultArena), cppTypes(gc), cppThreads(gc) {
		assert(Compiler::identifier == 0, L"Invalid ID for the compiler thread. Check CppTypes for errors!");

		// Since all types in the name tree need the Compiler thread, we need to create that a bit early.
		cppThreads.resize(1);
		{
			os::Thread t = mainThread(mode, *this);
			// Ensure 'mainThread' is executed before operator new.
			cppThreads[0] = new (Thread::First(*this)) Thread(t);
		}

		// Initialize the type system. This loads all types defined in the compiler.
		initTypes(*this, cppTypes);

		// Now, we can give the Compiler thread object a proper header with a type. Until this
		// point, doing as<> on that object will crash the system. However, that is not neccessary
		// this early during compiler startup.
		Thread::stormType(*this)->setType(cppThreads[0]);
	}

	Engine::~Engine() {
		// We need to remove the root this array implies before the Gc is destroyed.
		cppTypes.clear();
	}

	Type *Engine::cppType(nat id) const {
		return cppTypes[id];
	}

	Thread *Engine::cppThread(nat id) const {
		return cppThreads[id];
	}


	/**
	 * Interface which only exists in the compiler. Dynamic libraries have their own implementations
	 * which forward the calls to these implementations.
	 */

	Thread *DeclThread::thread(Engine &e) const {
		return e.cppThread(identifier);
	}

}
