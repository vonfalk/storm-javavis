#include "stdafx.h"
#include "Engine.h"
#include "Std.h"
#include "Exception.h"

namespace storm {

	Engine::Engine(const Path &root)
		: inited(false), rootPath(root), rootScope(null), arena(),
		  addRef(arena.addRef), release(arena.releaseRef),
		  allocRef(arena, L"alloc"), freeRef(arena, L"free"),
		  lazyCodeFn(arena, L"lazyUpdate") {

		specialCached.resize(specialCount);

		addRef.set(address(&Object::addRef));
		release.set(address(&Object::release));
		allocRef.set(address(&stormMalloc));
		freeRef.set(address(&stormFree));

		createStdTypes(*this, cached);

		// Now, all the types are created, so we can create packages!
		rootPkg = CREATE(Package, *this, root, *this);

		defScopeLookup = CREATE(ScopeLookup, *this);
		rootScope = new Scope(rootPkg);

		// And finally insert everything into their correct packages.
		addStdLib(*this);
		inited = true;
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

		t->release();

		TODO(L"Destroy these earlier if possible!"); // We can do this when we have proper threading.
		clear(toDestroy);

		delete rootScope;
	}

	void Engine::setSpecialBuiltIn(Special t, Auto<Type> z) {
		specialCached[nat(t)] = z;
	}

	Package *Engine::package(const Name &path, bool create) {
		Named *n = rootPkg->find(path);
		if (Package *pkg = as<Package>(n))
			return pkg;

		if (create)
			return createPackage(rootPkg.borrow(), path);

		return null;
	}

	Package *Engine::createPackage(Package *pkg, const Name &path, nat pos) {
		if (path.size() == pos)
			return pkg;

		Package *next = pkg->childPackage(path[pos]);
		if (next == null) {
			next = CREATE(Package, *this, path[pos], *this);
			pkg->add(next);
		}

		return createPackage(next, path, pos + 1);
	}

	void Engine::destroy(code::Binary *b) {
		toDestroy.push_back(b);
	}

}
