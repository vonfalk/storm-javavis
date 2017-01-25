#pragma once

#include "Gc.h"
#include "BootStatus.h"
#include "World.h"
#include "Scope.h"
#include "VTableCall.h"
#include "Code/Arena.h"
#include "Code/RefSource.h"
#include "Code/Reference.h"
#include "Utils/Lock.h"

// TODO: Do not depend on path!
#include "Utils/Path.h"

namespace storm {

	class Thread;
	class Package;
	class NameSet;
	class ScopeLookup;
	class StdIo;
	class TextReader;
	class TextWriter;

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
		inline bool has(BootStatus s) { return boot() >= s; }

		// Advance boot status.
		void advance(BootStatus to);

		// Run the function for all named objects in the cache.
		typedef void (*NamedFn)(Named *);
		void forNamed(NamedFn fn);

		// Get the one and only pointer handle for Object and TObject.
		const Handle &objHandle();
		const Handle &tObjHandle();

		// The threadgroup which all threads spawned from here shall belong to.
		os::ThreadGroup threadGroup;

		/**
		 * Packages.
		 */

		// Get the root package.
		Package *package();

		// Get a package relative to the root. If 'create', the package will be created. No
		// parameters are supported in the name.
		Package *package(SimpleName *name, bool create = false);

		// Find a NameSet relative to the root. If 'create', creates packages along the way.
		NameSet *nameSet(SimpleName *name, bool create = false);

		// Parse a string into a simple name and get the corresponding package.
		Package *package(const wchar *name);

		/**
		 * Scopes.
		 */

		// Get the root scope.
		Scope scope();

		// Get the default scope lookup.
		ScopeLookup *scopeLookup();

		/**
		 * Code generation.
		 */

		// The arena used for code generation for this platform.
		code::Arena *arena();

		// VTable call stubs.
		VTableCalls *vtableCalls();

		// Get the one and only Handle object for void.
		const Handle &voidHandle();

		// Well-known references:
		enum RefType {
			rEngine,
			rLazyCodeUpdate,
			rRuleThrow,
			// Allocate an object of the type given.
			rAlloc,
			// Execute as<T>.
			rAs,
			// # of bytes inside a vtable the object's vtable ptr is pointing.
			rVTableAllocOffset,
			// # of bytes inside TObject the thread is stored
			rTObjectOffset,
			// Access the 'atRaw' member of Map.
			rMapAt,
			// Acces the generic 'EnumType::toString'.
			rEnumToS,
			// Access to 'postRaw' and 'resultRaw' in Future.
			rFuturePost,
			rFutureResult,
			// Low-level helpers for spawning threads.
			rSpawnResult,
			rSpawnFuture,
			rSpawnLater,
			rSpawnParam,
			rAbortSpawn,
			// FnParams interaction.
			rFnParamsCtor,
			rFnParamsDtor,
			rFnParamsAdd,
			// Access to things inside FnBase.
			rFnNeedsCopy,
			rFnCall,
			rFnCreate,
			// Should be the last one.
			refCount,
		};

		// Get a reference to a function in the runtime.
		code::Ref ref(RefType ref);

		// Get the StdIo object.
		StdIo *stdIo();

		// Get stdin, stdout and stderr.
		TextReader *stdIn();
		TextWriter *stdOut();
		TextWriter *stdError();

		// Set std streams.
		void stdIn(TextReader *to);
		void stdOut(TextWriter *to);
		void stdError(TextWriter *to);

	private:
		// The compiler C++ world.
		World world;

		// How far along in the boot process?
		BootStatus bootStatus;

		/**
		 * GC:d objects.
		 */
		struct GcRoot {
			// Handles for Object and TObject.
			Handle *objHandle;
			Handle *tObjHandle;

			// Root package.
			Package *root;

			// Root scope lookup.
			ScopeLookup *rootLookup;

			// Arena.
			code::Arena *arena;

			// VTableCalls.
			code::VTableCalls *vtableCalls;

			// References.
			code::RefSource *refs[refCount];

			// Void handle.
			Handle *voidHandle;

			// Handles to readers for stdin, stdout and stderror.
			TextReader *stdIn;
			TextWriter *stdOut;
			TextWriter *stdError;
		};

		GcRoot o;

		// Root for GcRoot.
		Gc::Root *objRoot;

		// Standard IO thread.
		StdIo *ioThread;

		// Lock for initializing the io thread.
		util::Lock ioLock;

		// Create references.
		code::RefSource *createRef(RefType ref);

		// Cleanup code.
		void destroy();
	};

}
