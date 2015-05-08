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
#include "Lib/FnPtr.h"

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
		  fnPtrCopy(arena, L"FnPtrBase::needsCopy"), fnPtrCall(arena, L"FnPtrBase::callRaw"),
		  fnPtrCreate(arena, L"FnPtrBase::createRaw")
	{

		addRef.setPtr(address(&Object::addRef));
		release.setPtr(address(&Object::release));
		copyRefPtr.setPtr(address(&storm::copyRefPtr));
		releasePtr.setPtr(address(&storm::releasePtr));
		allocRef.setPtr(address(&stormMalloc));
		freeRef.setPtr(address(&stormFree));
		createStrFn.setPtr(address(&Str::createStr));
		asFn.setPtr(address(&objectAs));

		spawnLater.setPtr(address(&code::UThread::spawnLater));
		spawnParam.setPtr(address(&code::UThread::spawnParamMem));
		abortSpawn.setPtr(address(&code::UThread::abortSpawn));

		spawnResult.setPtr(address(&storm::spawnThreadResult));
		spawnFuture.setPtr(address(&storm::spawnThreadFuture));

		futureResult.setPtr(address(&FutureBase::resultRaw));

		fnParamsCtor.setPtr(address(&storm::fnParamsCtor));
		fnParamsDtor.setPtr(address(&storm::fnParamsDtor));
		fnParamsAdd.setPtr(address(&storm::fnParamsAdd));
		arrayToSMember.setPtr(address(&storm::valArrayToSMember));
		arrayToSAdd.setPtr(address(&storm::valArrayToSAdd));
		fnPtrCopy.setPtr(address(&storm::fnPtrNeedsCopy));
		fnPtrCall.setPtr(address(&storm::fnPtrCallRaw));
		fnPtrCreate.setPtr(address(&storm::FnPtrBase::createRaw));
	}

	Engine::Engine(const Path &root, ThreadMode mode)
		: inited(false), rootPath(root), rootScope(null), arena(), fnRefs(arena), engineRef(arena, L"engine") {

		engineRef.setPtr(this);

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
		// Tell the arena that we will remove everything shortly, so that it knows it is not worth
		// the effort to compute which RefSources it can remove early.
		arena.preShutdown();

		PLN("Shutting down...");

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
		if (!create)
			return as<Package>(find(rootPkg, path));

		return createPackage(rootPkg.borrow(), path);
	}

	Package *Engine::createPackage(Package *pkg, Par<Name> path, nat pos) {
		if (path->size() == pos)
			return pkg;

		assert(path->at(pos)->params.size() == 0);

		Named *nextNamed = pkg->find(path->at(pos));
		Package *next = as<Package>(nextNamed);
		if (next == null && nextNamed == null) {
			// Create a virtual package.
			Auto<Package> r = CREATE(Package, *this, path->at(pos)->name);
			next = r.borrow();
			pkg->add(r);
		} else if (next == null) {
			throw InternalError(L"Trying to create the package " + ::toS(path) +
								L" but " + nextNamed->identifier() + L" already exists!");
		}

		// Tail recursive, we can make a loop here!
		return createPackage(next, path, pos + 1);
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

}
