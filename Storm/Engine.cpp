#include "stdafx.h"
#include "Engine.h"
#include "Std.h"
#include "Exception.h"
#include "Thread.h"
#include "Wrap.h"
#include "VTable.h"
#include "Type.h"
#include "OS/UThread.h"
#include "OS/FnParams.h"
#include "Code/VTable.h"
#include "Shared/Str.h"
#include "Shared/Future.h"
#include "Lib/FnPtr.h"
#include "Shared/Io/Url.h"
#include "Shared/Map.h"

namespace storm {

	static void copyRefPtr(Object **to, Object **from) {
		*to = *from;
		(*to)->addRef();
	}

	static void releasePtr(Object **o) {
		(*o)->release();
	}


	FnRefs::FnRefs(code::Arena &arena)
		: addRef(arena.addRef), release(arena.releaseRef),
		  copyRefPtr(arena, L"copyRefPtr"), releasePtr(arena, L"releasePtr"),
		  allocRef(arena, L"alloc"), freeRef(arena, L"free"),
		  lazyCodeFn(arena, L"lazyUpdate"), createStrFn(arena, L"createStr"), asFn(arena, L"as"),
		  spawnLater(arena, L"spawnLater"), spawnParam(arena, L"spawnParam"),
		  abortSpawn(arena, L"abortSpawn"),
		  spawnResult(arena, L"spawnResult"), spawnFuture(arena, L"spawnFuture"),
		  futureResult(arena, L"futureResult"),
		  fnParamsCtor(arena, L"FnParams::ctor"), fnParamsDtor(arena, L"FnParams::dtor"),
		  fnParamsAdd(arena, L"FnParams::add"),
		  fnPtrCopy(arena, L"FnPtrBase::needsCopy"), fnPtrCall(arena, L"FnPtrBase::callRaw"),
		  fnPtrCreate(arena, L"FnPtrBase::createRaw"),
		  mapAccess(arena, L"MapBase::accessRaw")
	{

		addRef.setPtr(address(&Object::addRef));
		release.setPtr(address(&Object::release));
		copyRefPtr.setPtr(address(&storm::copyRefPtr));
		releasePtr.setPtr(address(&storm::releasePtr));
		allocRef.setPtr(address(&stormMalloc));
		freeRef.setPtr(address(&stormFree));
		createStrFn.setPtr(address(&Str::createStr));
		asFn.setPtr(address(&objectAs));

		spawnLater.setPtr(address(&os::UThread::spawnLater));
		spawnParam.setPtr(address(&os::UThread::spawnParamMem));
		abortSpawn.setPtr(address(&os::UThread::abortSpawn));

		spawnResult.setPtr(address(&storm::spawnThreadResult));
		spawnFuture.setPtr(address(&storm::spawnThreadFuture));

		futureResult.setPtr(address(&FutureBase::resultRaw));

		fnParamsCtor.setPtr(address(&storm::fnParamsCtor));
		fnParamsDtor.setPtr(address(&storm::fnParamsDtor));
		fnParamsAdd.setPtr(address(&storm::fnParamsAdd));
		fnPtrCopy.setPtr(address(&storm::fnPtrNeedsCopy));
		fnPtrCall.setPtr(address(&storm::fnPtrCallRaw));
		fnPtrCreate.setPtr(address(&storm::createRawFnPtr));

		mapAccess.setPtr(address(&storm::MapBase::accessRaw));
	}

	Engine::Engine(const Path &root, ThreadMode mode)
		: inited(false), rootPath(root), rootScope(null), arena(), fnRefs(arena), engineRef(arena, L"engine") {

		addRootToS(code::deVirtualize(address(&Object::toS), Object::cppVTable()));

		engineRef.setPtr(this);

		BuiltInLoader loader(*this, cached, storm::builtIn(), null);
		cppVTableSize = loader.vtableCapacity() + 20; // Hack for now. TODO: Fixme!
		vcalls = new VTableCalls(*this);
		specialCached.resize(specialCount);

		// Since some of the standard types are declared to be executed on the Compiler thread,
		// allocate some memory for the Compiler thread object, so that we can give those objects
		// a valid pointer that we will initialize later...
		Auto<Thread> compilerThread;
		this->threadMode = mode;
		if (mode == reuseMain) {
			os::Thread::initThread();
			compilerThread = CREATE_NOTYPE(Thread, *this, os::Thread::current());
		} else {
			compilerThread = CREATE_NOTYPE(Thread, *this);
		}
		Compiler::decl.force(*this, compilerThread.borrow());

		// Create the standard types in the compiler (like Type, among others).
		cached.resize(1);
		cached[0] = Type::createType(*this, L"Type", typeClass | typeManualSuper);
		loader.createTypes();

		// Make sure the 'compilerThread' object gets its type mark at last!
		SET_TYPE_LATE(compilerThread, Thread::stormType(*this));

		// Now, all the types are created, so we can create packages!
		Auto<Url> rootUrl = parsePath(*this, root.toS());
		rootPkg = CREATE(Package, *this, rootUrl);
		loader.setRoot(rootPkg.borrow());

		defScopeLookup = CREATE(ScopeLookup, *this);
		rootScope = new Scope(rootPkg);

		try {
			// And finally insert everything into their correct packages.
			addStdLib(*this, loader);
			inited = true;
		} catch (const Exception &e) {
			// For some reason this code crashes, so better echo any exceptions at this point!
			PVAR(e);
			delete rootScope;
			throw;
		} catch (...) {
			delete rootScope;
			throw;
		}
	}

	Engine::~Engine() {
		// NOTE: If we're launched with a separate thread as the compiler thread, we may fail here.

		// Tell the arena that we will remove everything shortly, so that it knows it is not worth
		// the effort to compute which RefSources it can remove early.
		arena.preShutdown();

		// Start shutting down the libraries now. This requires all types to be alive.
		loadedLibs.shutdown();

		// Remove any thread names so they do not mess things up when freeing the name tree.
		cppThreadNames.clear();

		// We need to destroy all types last, otherwise we will crash badly!
		vector<Auto<Type>> types = rootPkg->findTypes();

		// Clear out all types.
		rootPkg = null;
		rootScope->top = null; // keeps a reference to the root package.
		rootScope->lookup = null;
		defScopeLookup = null;

		// Find all unique types and stop their threads so that we can do a join() later on.
		set<Thread *> threads;
		for (nat i = 0; i < types.size(); i++) {
			RunOn on = types[i]->runOn();
			if (on.thread)
				threads.insert(on.thread->thread());
		}

		for (set<Thread *>::iterator i = threads.begin(), end = threads.end(); i != end; ++i) {
			(*i)->stopThread();
		}

		// Release more cached types. This needs to be above clearing other types.
		specialCached.clear();

		// Release any threads now.
		cppThreads.clear();

		// Wait until threads have terminated properly.
		threadGroup.join();

		// Keep the type type a little longer.
		Type *t = Type::stormType(*this);
		t->addRef();

		// Some of 'cached' may not be in 'types', since they may have been removed from the type
		// tree. This should not happen, but it is annoying when this crashes due to another error,
		// and this operation does not hurt performance by much at all.
		for (nat i = 0; i < types.size(); i++) {
			types[i]->clear();
		}

		for (nat i = 0; i < cached.size(); i++) {
			cached[i]->clear();
		}

		// Now, all types should contain nothing. Start clearing types!

		delete rootScope;
		loadedLibs.clearTypes();
		types.clear();
		cached.clear();

		delete vcalls;

		t->release();

		TODO(L"Destroy these earlier if possible!"); // We can do this when we have proper threading.
		clear(toDestroy);

		// Unload libs now. This dtor will use a map in here, causing a crash, unless we clear it first.
		loadedLibs.unload();

		Object::dumpLeaks();

		if (threadMode == reuseMain) {
			// We're assuming that we are destroyed from the same thread that we were created from.
			// This is usually the case.
			os::Thread::cleanThread();
		}
	}

	void Engine::setSpecialBuiltIn(Special t, Par<Type> z) {
		specialCached[nat(t)] = z;
	}

	Thread *Engine::thread(uintptr_t id, DeclThread::CreateFn fn) {
		ThreadMap::const_iterator i = cppThreads.find(id);
		if (i == cppThreads.end()) {
			Auto<Thread> t;
			if (fn) {
				t = CREATE(Thread, *this, fn);
			} else {
				t = CREATE(Thread, *this);
			}
			cppThreads.insert(make_pair(id, t));
			return t.borrow();
		} else {
			return i->second.borrow();
		}
	}

	void Engine::thread(uintptr_t id, Par<Thread> t) {
		assert(cppThreads.count(id) == 0, L"A thread with this id has already been created.");
		cppThreads.insert(make_pair(id, Auto<Thread>(t)));
	}

	NamedThread *Engine::threadName(uintptr_t id) {
		ThreadNameMap::const_iterator i = cppThreadNames.find(id);
		if (i == cppThreadNames.end())
			return null;
		else
			return i->second.borrow();
	}

	void Engine::threadName(uintptr_t id, Par<NamedThread> t) {
		cppThreadNames.insert(make_pair(id, Auto<NamedThread>(t)));
	}

	Package *Engine::package(const String &path, bool create) {
		Auto<SimpleName> name = parseSimpleName(*this, path);
		return package(name, create);
	}

	Package *Engine::package(Par<SimpleName> path, bool create) {
		return package(rootPkg, path, create);
	}

	Package *Engine::package(Par<Package> rel, Par<SimpleName> path, bool create) {
		if (!create)
			return steal(find(rel, path)).as<Package>().borrow();
		else
			return createPackage(rel.borrow(), path);
	}

	NameSet *Engine::nameSet(Par<SimpleName> path, bool create) {
		return package(rootPkg, path, create);
	}

	NameSet *Engine::nameSet(Par<Package> rel, Par<SimpleName> path, bool create) {
		NameSet *found = steal(find(rel, path)).as<NameSet>().borrow();
		if (found)
			return found;

		if (create)
			return createPackage(rel.borrow(), path);

		return null;
	}

	Package *Engine::createPackage(Package *pkg, Par<SimpleName> path, nat pos) {
		if (path->count() <= pos)
			return pkg;

		assert(path->at(pos)->empty());

		Auto<SimplePart> part = CREATE(SimplePart, *this, path->at(pos)->name);
		Auto<Named> nextNamed = pkg->find(part);
		Auto<Package> next = nextNamed.as<Package>();
		if (!next && !nextNamed) {
			// Create a virtual package.
			next = CREATE(Package, *this, path->at(pos)->name);
			pkg->add(next);
		} else if (next == null) {
			throw InternalError(L"Trying to create the package " + ::toS(path) +
								L" but " + nextNamed->identifier() + L" already exists!");
		}

		// Tail recursive, we can make a loop here!
		return createPackage(next.borrow(), path, pos + 1);
	}

	Package *Engine::rootPackage() {
		return rootPkg.borrow();
	}

	void Engine::destroy(code::Binary *b) {
		if (b) {
			// Todo: this could be solved better...
			code::RefSource *src = new code::RefSource(arena, L"*keepalive*");
			src->set(b);
			toDestroy.push_back(src);
		}
	}

	code::Ref Engine::virtualCall(VTablePos pos) const {
		return vcalls->call(pos);
	}

	bool Engine::rootToS(void *fn) {
		return toSRoot.count(fn) != 0;
	}

	void Engine::addRootToS(void *addr) {
		map<void *, nat>::iterator i = toSRoot.find(addr);
		if (i == toSRoot.end()) {
			toSRoot.insert(make_pair(addr, nat(1)));
		} else {
			i->second++;
		}
	}

	void Engine::removeRootToS(void *addr) {
		map<void *, nat>::iterator i = toSRoot.find(addr);
		if (i == toSRoot.end()) {
			WARNING(L"Trying to remove an unknown toS function.");
		} else if (i->second <= 1) {
			toSRoot.erase(i);
		} else {
			i->second--;
		}
	}

}
