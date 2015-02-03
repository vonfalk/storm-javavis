#include "stdafx.h"
#include "Engine.h"
#include "Std.h"
#include "Exception.h"

namespace storm {

#ifdef DEBUG
	static void CODECALL dbgPrint(Int v) {
		PLN("Debug: " << v);
	}
#endif

	Engine::Engine(const Path &root)
		: inited(false), rootPath(root), rootScope(null), arena(),
		  addRef(arena.addRef), release(arena.releaseRef),
		  allocRef(arena, L"alloc"), freeRef(arena, L"free"),
#ifdef DEBUG
		  dbgPrint(arena, L"dbgPrint"),
#endif
		  lazyCodeFn(arena, L"lazyUpdate") {

		cppVTableSize = maxVTableCount();
		vcalls = new VTableCalls(*this);
		specialCached.resize(specialCount);

		addRef.set(address(&Object::addRef));
		release.set(address(&Object::release));
		allocRef.set(address(&stormMalloc));
		freeRef.set(address(&stormFree));

#ifdef DEBUG
		dbgPrint.set(address(&storm::dbgPrint));
#endif

		createStdTypes(*this, cached);

		// Now, all the types are created, so we can create packages!
		rootPkg = CREATE(Package, *this, root, *this);

		defScopeLookup = CREATE(ScopeLookup, *this);
		rootScope = new Scope(rootPkg);

		try {
			// And finally insert everything into their correct packages.
			addStdLib(*this);
			inited = true;
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
			pkg->add(r.borrow());
			next = r.borrow();
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
