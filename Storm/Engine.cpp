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
		  lazyCodeFn(arena, L"lazyUpdate"), createStrFn(arena, L"createStr"), asFn(arena, L"as"),
		  spawnLater(arena, L"spawnLater"), spawnParam(arena, L"spawnParam"),
		  abortSpawn(arena, L"abortSpawn"),
		  spawnResult(arena, L"spawnResult"), spawnFuture(arena, L"spawnFuture"),
		  futureResult(arena, L"futureResult"),
		  fnParamsCtor(arena, L"FnParams::ctor"), fnParamsDtor(arena, L"FnParams::dtor"),
		  fnParamsAdd(arena, L"FnParams::add"),
		  arrayToSMember(arena, L"valArrayToSMember"), arrayToSAdd(arena, L"valArrayToSAdd"),
		  fnPtrCopy(arena, L"FnPtrBase::needsCopy"), fnPtrCall(arena, L"FnPtrBase::callRaw")
	{

		addRef.set(address(&Object::addRef));
		release.set(address(&Object::release));
		copyRefPtr.set(address(&storm::copyRefPtr));
		releasePtr.set(address(&storm::releasePtr));
		allocRef.set(address(&stormMalloc));
		freeRef.set(address(&stormFree));
		createStrFn.set(address(&Str::createStr));
		asFn.set(address(&objectAs));

		spawnLater.set(address(&code::UThread::spawnLater));
		spawnParam.set(address(&code::UThread::spawnParamMem));
		abortSpawn.set(address(&code::UThread::abortSpawn));

		spawnResult.set(address(&storm::spawnThreadResult));
		spawnFuture.set(address(&storm::spawnThreadFuture));

		futureResult.set(address(&FutureBase::resultRaw));

		fnParamsCtor.set(address(&storm::fnParamsCtor));
		fnParamsDtor.set(address(&storm::fnParamsDtor));
		fnParamsAdd.set(address(&storm::fnParamsAdd));
		arrayToSMember.set(address(&storm::valArrayToSMember));
		arrayToSAdd.set(address(&storm::valArrayToSAdd));
		fnPtrCopy.set(address(&storm::fnPtrNeedsCopy));
		fnPtrCall.set(address(&storm::fnPtrCallRaw));
	}

	Engine::Engine(const Path &root, ThreadMode mode)
		: inited(false), rootPath(root), rootScope(null), arena(), fnRefs(arena), engineRef(arena, L"engine") {

		engineRef.set(this);

		cppVTableSize = maxVTableCount();
		vcalls = new VTableCalls(*this);
		specialCached.resize(specialCount);

		// Since some of the standard types are declared to be executed on the Compiler thread,
		// allocate some memory for the Compiler thread object, so that we can give those objects
		// a valid pointer that we will initialize later...
		Auto<Thread> compilerThread;
		if (mode == reuseMain) {
			compilerThread = CREATE_NOTYPE(Thread, *this, code::Thread::current());
		} else {
			compilerThread = CREATE_NOTYPE(Thread, *this);
		}
		Compiler::force(*this, compilerThread.borrow());

		// Create the standard types in the compiler (like Type, among others).
		createStdTypes(*this, cached);

		// Make sure the 'compilerThread' object gets its type mark at last!
		SET_TYPE_LATE(compilerThread, Thread::stormType(*this));

		// Now, all the types are created, so we can create packages!
		Auto<Url> rootUrl = parsePath(*this, root.toS());
		rootPkg = CREATE(Package, *this, rootUrl);

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

		// We need to destroy all types last, otherwise we will crash badly!
		vector<Auto<Type>> types = rootPkg->findTypes();

		rootPkg = null;
		rootScope->top = null; // keeps a reference to the root package.
		rootScope->lookup = null;
		defScopeLookup = null;

		// Release more cached types. This needs to be above clearing other types.
		specialCached.clear();

		// Release any threads now.
		threads.clear();

		// Keep the type type a little longer.
		Type *t = Type::stormType(*this);
		t->addRef();

		// All of 'cached' should be in 'types' as well!
		for (nat i = 0; i < types.size(); i++) {
			types[i]->clear();
		}
		types.clear();
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
			Auto<Package> r = CREATE(Package, *this, path->at(pos)->name);
			next = r.borrow();
			pkg->add(r);
		}

		return createPackage(next, path, pos + 1);
	}

	Package *Engine::rootPackage() {
		return rootPkg.borrow();
	}

	void Engine::destroy(code::Binary *b) {
		toDestroy.push_back(b);
	}

	code::Ref Engine::virtualCall(VTablePos pos) const {
		return vcalls->call(pos);
	}

}
