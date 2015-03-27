#include "stdafx.h"
#include "Engine.h"
#include "Std.h"
#include "Exception.h"
#include "Thread.h"
#include "Wrap.h"
#include "Code/UThread.h"
#include "Code/FnParams.h"
#include "Lib/Str.h"
#include "Lib/Future.h"

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
		  lazyCodeFn(arena, L"lazyUpdate"), createStrFn(arena, L"createStr"),
		  spawnLater(arena, L"spawnLater"), spawnParam(arena, L"spawnParam"),
		  abortSpawn(arena, L"abortSpawn"), spawn(arena, L"spawn"), spawnResult(arena, L"spawnResult"),
		  futurePost(arena, L"futurePost"), futureResult(arena, L"futureResult"),
		  fnParamsCtor(arena, L"FnParams::ctor"), fnParamsDtor(arena, L"FnParams::dtor"),
		  fnParamsAdd(arena, L"FnParams::add")
	{

		addRef.set(address(&Object::addRef));
		release.set(address(&Object::release));
		copyRefPtr.set(address(&storm::copyRefPtr));
		releasePtr.set(address(&storm::releasePtr));
		allocRef.set(address(&stormMalloc));
		freeRef.set(address(&stormFree));
		createStrFn.set(address(&Str::createStr));

		spawnLater.set(address(&code::UThread::spawnLater));
		spawnParam.set(address(&code::UThread::spawnParamMem));
		abortSpawn.set(address(&code::UThread::abortSpawn));

		spawn.set(address(&storm::spawnThread));
		spawnResult.set(address(&storm::spawnThreadResult));

		futurePost.set(address(&FutureBase::postRaw));
		futureResult.set(address<void (CODECALL FutureBase::*)(void *)>(&FutureBase::resultRaw));

		fnParamsCtor.set(address(&storm::fnParamsCtor));
		fnParamsDtor.set(address(&storm::fnParamsDtor));
		fnParamsAdd.set(address(&storm::fnParamsAdd));
	}

	Engine::Engine(const Path &root, ThreadMode mode)
		: inited(false), rootPath(root), rootScope(null), arena(), fnRefs(arena) {

		cppVTableSize = maxVTableCount();
		vcalls = new VTableCalls(*this);
		specialCached.resize(specialCount);

		createStdTypes(*this, cached);

		// If we are to reuse the calling thread, we have to set the compiler thread up
		// before we call 'addStdLib' below. Otherwise a new thread will be created.
		if (mode == reuseMain) {
			Auto<Thread> t = CREATE(Thread, *this, code::Thread::current());
			Compiler::force(*this, t.borrow());
		}

		// Now, all the types are created, so we can create packages!
		rootPkg = CREATE(Package, *this, root, *this);

		defScopeLookup = CREATE(ScopeLookup, *this);
		rootScope = new Scope(rootPkg);

		try {
			// And finally insert everything into their correct packages.
			addStdLib(*this);
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
		// Disable warnings for unneeded references at this point... We do not know the correct
		// destruction order, and it would not be worth it to compute it.
		arena.preShutdown();

		rootPkg = null;
		rootScope->top = null; // keeps a reference to the root package.
		rootScope->lookup = null;
		defScopeLookup = null;

		// Release more cached types. This needs to be above clearing other types.
		specialCached.clear();

		// Release any threads now.
		threads.clear();

		// Keep the type type a little longer.
		Type *t = Type::type(*this);
		t->addRef();

		for (nat i = 0; i < cached.size(); i++) {
			cached[i]->clear();
		}
		cached.clear();
		delete vcalls;

		t->release();

		TODO(L"Destroy these earlier if possible!"); // We can do this when we have proper threading.
		clear(toDestroy);

		delete rootScope;
	}

	void Engine::setSpecialBuiltIn(Special t, Par<Type> z) {
		specialCached[nat(t)] = z;
	}

	Thread *Engine::thread(uintptr_t id) {
		ThreadMap::const_iterator i = threads.find(id);
		if (i == threads.end()) {
			Auto<Thread> t = CREATE(Thread, *this);
			threads.insert(make_pair(id, t));
			return t.borrow();
		} else {
			return i->second.borrow();
		}
	}

	void Engine::thread(uintptr_t id, Par<Thread> t) {
		assert(threads.count(id) == 0, L"A thread with this id has already been created.");
		threads.insert(make_pair(id, Auto<Thread>(t)));
	}

	Package *Engine::package(const String &path) {
		Auto<Name> name = parseSimpleName(*this, path);
		return package(name);
	}

	Package *Engine::package(Par<Name> path, bool create) {
		Named *n = find(rootPkg, path);
		if (Package *pkg = as<Package>(n))
			return pkg;

		if (create)
			return createPackage(rootPkg.borrow(), path);

		return null;
	}

	Package *Engine::createPackage(Package *pkg, Par<Name> path, nat pos) {
		if (path->size() == pos)
			return pkg;

		assert(path->at(pos)->params.size() == 0);

		Package *next = as<Package>(pkg->find(path->at(pos)));
		if (next == null) {
			Auto<Package> r = CREATE(Package, *this, path->at(pos)->name, *this);
			next = r.borrow();
			pkg->add(r);
		}

		return createPackage(next, path, pos + 1);
	}

	void Engine::destroy(code::Binary *b) {
		toDestroy.push_back(b);
	}

	code::Ref Engine::virtualCall(VTablePos pos) const {
		return vcalls->call(pos);
	}

}
