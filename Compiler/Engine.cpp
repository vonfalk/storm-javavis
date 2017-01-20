#include "stdafx.h"
#include "Engine.h"
#include "Init.h"
#include "Type.h"
#include "Package.h"
#include "Code.h"
#include "Function.h"
#include "Core/Hash.h"
#include "Core/Thread.h"
#include "Core/Str.h"
#include "Core/Map.h"
#include "Core/StrBuf.h"
#include "Core/Handle.h"
#include "Syntax/Node.h"
#include "Utils/Memory.h"

// Only included from here:
#include "OS/SharedMaster.h"

namespace storm {

	// Default arena size. 256 MB should be enough for a while at least.
	static const size_t defaultArena = 256 * 1024 * 1024;

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
		gc(defaultArena, defaultFinalizer), world(gc), objRoot(null) {

		bootStatus = bootNone;

		// Initialize the roots we need.
		memset(&o, 0, sizeof(GcRoot));
		objRoot = gc.createRoot(&o, sizeof(GcRoot) / sizeof(void *));

		assert(Compiler::identifier == 0, L"Invalid ID for the compiler thread. Check CppTypes for errors!");

		try {
			// Since all types in the name tree need the Compiler thread, we need to create that a bit early.
			world.threads.resize(1);
			{
				os::Thread t = mainThread(mode, *this);
				// Ensure 'mainThread' is executed before operator new.
				world.threads[0] = new (Thread::First(*this)) Thread(t);
			}

			// Initialize the type system. This loads all types defined in the compiler.
			initTypes(*this, world);

			// Now, we can give the Compiler thread object a proper header with a type. Until this
			// point, doing as<> on that object will crash the system. However, that is not neccessary
			// this early during compiler startup.
			Thread::stormType(*this)->setType(world.threads[0]);

			// Set proper paths for the now created packages.
			o.root->setUrl(parsePath(*this, toS(root).c_str()));

			// Done booting.
			advance(bootDone);
		} catch (...) {
			destroy();
			throw;
		}
	}

	Engine::~Engine() {
		// Do not place anything directly here, we need the cleanup when creation fails.
		destroy();
	}

	void Engine::destroy() {
		advance(bootShutdown);

		// We need to remove the root this array implies before the Gc is destroyed.
		world.clear();

		gc.destroyRoot(objRoot);
	}

	Type *Engine::cppType(Nat id) const {
		return world.types[id];
	}

	TemplateList *Engine::cppTemplate(Nat id) const {
		if (id >= world.templates.count())
			return null;
		return world.templates[id];
	}

	Thread *Engine::cppThread(Nat id) const {
		return world.threads[id];
	}

	void Engine::forNamed(NamedFn fn) {
		world.forNamed(fn);
	}

	/**
	 * Helpers for the pointer handle.
	 */

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

	const Handle &Engine::objHandle() {
		if (!o.objHandle) {
			o.objHandle = new (*this) Handle();
			o.objHandle->size = sizeof(void *);
			o.objHandle->locationHash = false;
			o.objHandle->gcArrayType = &pointerArrayType;
			o.objHandle->copyFn = null; // No special function, use memcpy.
			o.objHandle->deepCopyFn = &objDeepCopy;
			o.objHandle->toSFn = &objToS;
			o.objHandle->hashFn = &objHash;
			o.objHandle->equalFn = &objEqual;
		}
		return *o.objHandle;
	}

	static void tObjToS(const void *obj, StrBuf *to) {
		const TObject *o = *(const TObject **)obj;
		*to << o;
	}

	static Nat tObjHash(const void *obj) {
		const void *ptr = *(const void **)obj;
		return ptrHash(ptr);
	}

	static Bool tObjEqual(const void *a, const void *b) {
		const void *aa = *(const void **)a;
		const void *bb = *(const void **)b;
		return aa == bb;
	}

	const Handle &Engine::tObjHandle() {
		if (!o.tObjHandle) {
			o.tObjHandle = new (*this) Handle();
			o.tObjHandle->size = sizeof(void *);
			o.tObjHandle->locationHash = true;
			o.tObjHandle->gcArrayType = &pointerArrayType;
			o.tObjHandle->copyFn = null; // No special function, use memcpy.
			o.tObjHandle->deepCopyFn = null; // No need for deepCopy.
			o.tObjHandle->toSFn = &tObjToS;
			o.tObjHandle->hashFn = &tObjHash;
			o.tObjHandle->equalFn = &tObjEqual;
		}
		return *o.tObjHandle;
	}

	void Engine::advance(BootStatus to) {
		assert(to >= bootStatus, L"Trying to devolve the boot status.");
		bootStatus = to;
	}

	ScopeLookup *Engine::scopeLookup() {
		if (!o.rootLookup)
			o.rootLookup = new (*this) ScopeLookup();

		return o.rootLookup;
	}

	Scope Engine::scope() {
		return Scope(package(), scopeLookup());
	}

	code::Arena *Engine::arena() {
		if (!o.arena)
			o.arena = code::arena(*this);

		return o.arena;
	}

	VTableCalls *Engine::vtableCalls() {
		if (!o.vtableCalls)
			o.vtableCalls = new (*this) VTableCalls();

		return o.vtableCalls;
	}

	static const GcType voidArrayType = {
		GcType::tArray,
		null,
		null,
		0,
		0, {}
	};

	static void voidToS(const void *, StrBuf *to) {
		*to << L"void";
	}

	static Nat voidHash(const void *) {
		return 0;
	}

	static Bool voidEqual(const void *, const void *) {
		return true;
	}

	const Handle &Engine::voidHandle() {
		if (!o.voidHandle) {
			o.voidHandle = new (*this) Handle();
			o.voidHandle->size = 0;
			o.voidHandle->gcArrayType = &voidArrayType;
			o.voidHandle->toSFn = &voidToS;
			o.voidHandle->hashFn = &voidHash;
			o.voidHandle->equalFn = &voidEqual;
		}

		return *o.voidHandle;
	}

	code::Ref Engine::ref(RefType ref) {
		code::RefSource **r = &o.refs[ref];
		if (!*r)
			*r = createRef(ref);

		return code::Ref(*r);
	}

	static void fnParamsCtor(void *memory, void *ptr) {
		new (memory) os::FnParams(ptr);
	}

	static void fnParamsDtor(os::FnParams *o) {
		o->~FnParams();
	}

	static void fnParamsAdd(os::FnParams *obj, os::FnParams::CopyFn copy, os::FnParams::DestroyFn destroy,
							size_t size, bool isFloat, const void *value) {
		obj->add(copy, destroy, size, isFloat, value);
	}

	static void *allocType(Type *t) {
		return runtime::allocObject(0, t);
	}

	static RootObject *stormAs(RootObject *in, Type *type) {
		if (!in)
			return null;
		if (in->isA(type))
			return in;
		return null;
	}

	code::RefSource *Engine::createRef(RefType ref) {
		switch (ref) {
		case rEngine:
			return arena()->externalSource(L"engine", this);
		case rLazyCodeUpdate:
			return arena()->externalSource(L"lazyUpdater", &LazyCode::updateCode);
		case rRuleThrow:
			return arena()->externalSource(L"ruleThrow", address(&syntax::Node::throwError));
		case rAlloc:
			return arena()->externalSource(L"alloc", &allocType);
		case rAs:
			return arena()->externalSource(L"as", &stormAs);
		case rVTableAllocOffset:
			return arena()->externalSource(L"vtableAllocOffset", (const void *)VTableCpp::vtableAllocOffset());
		case rTObjectOffset:
			return arena()->externalSource(L"threadOffset", (const void *)OFFSET_OF(TObject, thread));
		case rMapAt:
			return arena()->externalSource(L"MapBase::atRaw", address(&MapBase::atRaw));
		case rSpawnResult:
			return arena()->externalSource(L"spawnResult", &spawnThreadResult);
		case rSpawnFuture:
			assert(false, L"Implement me!");
			// return arena()->externalSource(L"spawnFuture", &spawnThreadFuture);
		case rSpawnLater:
			return arena()->externalSource(L"spawnLater", address(&os::UThread::spawnLater));
		case rSpawnParam:
			return arena()->externalSource(L"spawnParam", address(&os::UThread::spawnParamMem));
		case rAbortSpawn:
			return arena()->externalSource(L"abortSpawn", address(&os::UThread::abortSpawn));
		case rFnParamsCtor:
			return arena()->externalSource(L"(x) FnParams(y)", address(&fnParamsCtor));
		case rFnParamsDtor:
			return arena()->externalSource(L"~FnParams()", address(&fnParamsDtor));
		case rFnParamsAdd:
			return arena()->externalSource(L"fnParams->add", address(&fnParamsAdd));
		default:
			assert(false, L"Unknown reference: " + ::toS(ref));
		}
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

	Package *Engine::package(const wchar *name) {
		SimpleName *n = parseSimpleName(*this, name);
		return package(n, false);
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
