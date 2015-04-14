#pragma once
#include "Utils/Path.h"
#include "Name.h"
#include "Package.h"
#include "Scope.h"
#include "VTablePos.h"
#include "Code/Arena.h"
#include "Code/Binary.h"

namespace storm {

	class VTableCalls;

	/**
	 * Some special types.
	 */
	enum Special {
		specialInt = 0,
		specialNat,
		specialByte,
		specialBool,

		// Array<Str>, we need this during startup in Url.
		specialArrayStr,

		// last
		specialCount,
	};

	/**
	 * References to various functions in the compiler.
	 */
	class FnRefs {
	public:
		// Create.
		FnRefs(code::Arena &arena);

		// Reference to the addRef and release functions.
		code::RefSource &addRef, &release;

		// Add/release references to a pointer. Ie Object **.
		code::RefSource copyRefPtr, releasePtr;

		// Reference to the memory allocation function.
		code::RefSource allocRef, freeRef;

		// Other references.
		code::RefSource lazyCodeFn, createStrFn, asFn;

		// Things we need to call another thread.
		code::RefSource spawnLater, spawnParam, abortSpawn;
		code::RefSource spawnResult, spawnFuture;

		// Interface to the FnParams needed.
		code::RefSource fnParamsCtor, fnParamsDtor, fnParamsAdd;

		// Some helpers for a Future object.
		code::RefSource futureResult;

		// Helpers for the FnPtr object.
		code::RefSource fnPtrCopy, fnPtrCall;
	};

	/**
	 * Defines the root object of the compiler. This object contains
	 * everything needed by the compiler itself, and shall be kept alive
	 * as long as anything from the compiler is used.
	 * Two separate instances of this class can be used as two completely
	 * separate runtime environments.
	 */
	class Engine : NoCopy {
	public:
		// Threading mode for the engine:
		enum ThreadMode {
			// Create a new main thread for the compiler (this is the storm::Compiler thread) to
			// use for compiling code. This means that the compiler will manage itself, but care
			// must be taken not to manipulate anything below the engine in another thread.
			newMain,

			// Reuses the caller of the constructor as the compiler thread (storm::Compiler). This
			// makes it easier to interface with the compiler itself, since calls can be made
			// directly into the compiler, but care must be taken to allow the compiler to process
			// any messages sent to the compiler by calling UThread::leave(), or make the thread
			// wait in a code::Lock or code::Sema for events.
			reuseMain,
		};

		// Create the engine.
		// 'root' is the location of the root package directory on disk. The
		// package 'core' is assumed to be found as a subdirectory of the given root path.
		// TODO: Remove our depency on util::Path!
		Engine(const Path &root, ThreadMode mode);

		~Engine();

		// Find the given package. Returns null on failure. If 'create' is
		// true, then all packages that does not yet exist are created. All returns
		// a borrowed ptr.
		Package *package(Par<Name> path, bool create = false);
		Package *package(const String &name);
		Package *rootPackage();

		// Get a built-in type.
		inline Type *builtIn(nat id) const { return cached[id].borrow(); }

		// Special built-in types.
		inline Type *specialBuiltIn(Special t) const { return specialCached[nat(t)].borrow(); }
		void setSpecialBuiltIn(Special t, Par<Type> z);

		// Find threads. The threads declared by STORM_THREAD are looked up this way. They live
		// as long as the compiler does. Returns a borrowed ptr.
		Thread *thread(uintptr_t id);

		// Set a specific thread to be used as a specific id.
		void thread(uintptr_t id, Par<Thread> thread);

		// Get the default scope lookup.
		inline Auto<ScopeLookup> scopeLookup() { assert(defScopeLookup); return defScopeLookup; }

		// Get the default scope for the root package.
		inline Scope *scope() { assert(rootScope != null); return rootScope; }

		// Initialized?
		inline bool initialized() { return inited; }

		// Arena. NOTE: Place this before 'engineRef' and 'fnRefs' since C++ compilers generally re-order
		// initializations.
		code::Arena arena;

		// Named reference to the engine.
		code::RefSource engineRef;

		// Built-in functions.
		FnRefs fnRefs;

		// Get a reference to a virtual function call.
		code::Ref virtualCall(VTablePos pos) const;

		// Delete old code later.
		void destroy(code::Binary *binary);

		// Get the maxium size needed for any C++ vtable.
		inline nat maxCppVTable() const { return cppVTableSize; }

	private:
		// Path to root directory.
		Path rootPath;

		// Root package.
		Auto<Package> rootPkg;

		// Create the package (recursive).
		Package *createPackage(Package *pkg, Par<Name> path, nat at = 0);

		// Root scope.
		Scope *rootScope;

		// Scope lookup.
		Auto<ScopeLookup> defScopeLookup;

		// Cached types.
		vector<Auto<Type> > cached;
		vector<Auto<Type> > specialCached;

		// Threads.
		typedef hash_map<uintptr_t, Auto<Thread> > ThreadMap;
		ThreadMap threads;

		// Maxium C++ VTable size.
		nat cppVTableSize;

		// Initialized?
		bool inited;

		// Binary objects to destroy. TODO: Be more eager!
		vector<code::Binary *> toDestroy;

		// The cache of virtual function call stubs.
		VTableCalls *vcalls;

		// Set T to the type, reporting any errors.
		void setType(Type *&t, const String &name);
	};

}
