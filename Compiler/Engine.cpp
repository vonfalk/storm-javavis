#include "stdafx.h"
#include "Engine.h"
#include "Init.h"
#include "Type.h"
#include "Package.h"
#include "Core/Thread.h"
#include "Core/Str.h"
#include "Core/StrBuf.h"
#include "Core/Handle.h"

// Only included from here:
#include "OS/SharedMaster.h"

namespace storm {

	// Default arena size. 32 MB should be enough for a while at least.
	static const size_t defaultArena = 32 * 1024 * 1024;

	// Default finalizer interval.
	static const nat defaultFinalizer = 1000;

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

	Engine::Engine(const Path &root, ThreadMode mode) :
		gc(defaultArena, defaultFinalizer), cppTypes(gc), cppTemplates(gc),
		cppThreads(gc), objRoot(null) {

		bootStatus = bootNone;

		// Initialize the roots we need.
		memset(&o, 0, sizeof(GcRoot));
		objRoot = gc.createRoot(&o, sizeof(GcRoot) / sizeof(void *));

		assert(Compiler::identifier == 0, L"Invalid ID for the compiler thread. Check CppTypes for errors!");

		// Since all types in the name tree need the Compiler thread, we need to create that a bit early.
		cppThreads.resize(1);
		{
			os::Thread t = mainThread(mode, *this);
			// Ensure 'mainThread' is executed before operator new.
			cppThreads[0] = new (Thread::First(*this)) Thread(t);
		}

		// Initialize the type system. This loads all types defined in the compiler.
		initTypes(*this, cppTypes, cppTemplates);

		// Now, we can give the Compiler thread object a proper header with a type. Until this
		// point, doing as<> on that object will crash the system. However, that is not neccessary
		// this early during compiler startup.
		Thread::stormType(*this)->setType(cppThreads[0]);

		// Done booting.
		advance(bootDone);
	}

	Engine::~Engine() {
		// We need to remove the root this array implies before the Gc is destroyed.
		cppTypes.clear();

		gc.destroyRoot(objRoot);
	}

	Type *Engine::cppType(Nat id) const {
		return cppTypes[id];
	}

	TemplateList *Engine::cppTemplate(Nat id) const {
		if (id >= cppTemplates.count())
			return null;
		return cppTemplates[id];
	}

	Thread *Engine::cppThread(Nat id) const {
		return cppThreads[id];
	}

	void Engine::forNamed(NamedFn fn) {
		for (nat i = 0; i < cppTypes.count(); i++)
			(*fn)(cppTypes[i]);
		for (nat i = 0; i < cppTemplates.count(); i++)
			cppTemplates[i]->forNamed(fn);
	}

	/**
	 * Helpers for the pointer handle.
	 */

	static GcType ptrArray = {
		GcType::tArray,
		null,
		null,
		sizeof(void *),
		1,
		{ 0 },
	};

	static void objDeepCopy(void *obj, CloneEnv *env) {
		Object *o = *(Object **)obj;
		o->deepCopy(env);
	}

	static void objToS(const void *obj, StrBuf *to) {
		const Object *o = *(const Object **)obj;
		*to << o;
	}

	static Nat objHash(const void *obj) {
		const Object *o = *(const Object **)obj;
		return o->hash();
	}

	static Bool objEqual(const void *a, const void *b) {
		Object *ao = *(Object **)a;
		Object *bo = *(Object **)b;
		return ao->equals(bo);
	}

	const Handle &Engine::ptrHandle() {
		if (!o.pHandle) {
			o.pHandle = new (*this) Handle();
			o.pHandle->size = sizeof(void *);
			o.pHandle->gcArrayType = &ptrArray;
			o.pHandle->copyFn = null; // No special function, use memcpy.
			o.pHandle->deepCopyFn = &objDeepCopy;
			o.pHandle->toSFn = &objToS;
			o.pHandle->hashFn = &objHash;
			o.pHandle->equalFn = &objEqual;
		}
		return *o.pHandle;
	}

	void Engine::advance(BootStatus to) {
		assert(to >= bootStatus, L"Trying to devolve the boot status.");
		bootStatus = to;
	}

	Package *Engine::package() {
		if (!o.root) {
			assert(has(bootTypes), L"Can not create packages yet.");
			o.root = new (*this) Package(new (*this) Str(L"<root>"));
		}
		return o.root;
	}

	Package *Engine::package(SimpleName *name, bool create) {
		Package *now = package();

		for (nat i = 0; i < name->count(); i++) {
			SimplePart *p = name->at(i);

			// Not supported.
			if (p->params->any())
				return null;

			Named *next = now->find(p);
			if (!next) {
				Package *pkg = new (*this) Package(p->name);
				now->add(pkg);
				now = pkg;
			} else if (Package *p = as<Package>(next)) {
				now = p;
			} else {
				return null;
			}
		}

		return now;
	}

	NameSet *Engine::nameSet(SimpleName *name, bool create) {
		NameSet *now = package();

		for (nat i = 0; i < name->count(); i++) {
			SimplePart *p = name->at(i);

			// Not supported.
			if (p->params->any())
				return null;

			Named *next = now->find(p);
			if (!next) {
				Package *pkg = new (*this) Package(p->name);
				now->add(pkg);
				now = pkg;
			} else if (NameSet *p = as<NameSet>(next)) {
				now = p;
			} else {
				return null;
			}
		}

		return now;
	}


	/**
	 * Interface which only exists in the compiler. Dynamic libraries have their own implementations
	 * which forward the calls to these implementations.
	 */

	Thread *DeclThread::thread(Engine &e) const {
		return e.cppThread(identifier);
	}

}
