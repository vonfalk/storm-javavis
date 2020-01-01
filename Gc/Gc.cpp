#include "stdafx.h"
#include "Gc.h"
#include "Utils/Memory.h"

/**
 * Include all possible GC implementations. Only one will be selected.
 */
#include "Zero.h"
#include "MPS/Impl.h"
#include "SMM/Impl.h"

#ifndef STORM_HAS_GC
// If this happens, an unselected GC has been selected in Core/Storm.h.
#error "No GC selected!"
#endif

namespace storm {

	class Gc::Impl : public GcImpl {
	public:
		Impl(size_t initialArena, nat finalizationInterval)
			: GcImpl(initialArena, finalizationInterval), destroyed(false) {}

		~Impl() {
			destroy();
		}

		// Data for a thread.
		struct ThreadData {
			size_t refs;
			GcImpl::ThreadData data;
		};

		// Remember all threads.
		typedef map<uintptr_t, ThreadData> ThreadMap;
		ThreadMap threads;

		// Lock for manipulating the attached threads.
		util::Lock threadLock;

		// Destroyed already?
		Bool destroyed;

		// Helpers.
		void destroy();
		void attachThread();
		void reattachThread(const os::Thread &thread);
		void detachThread(const os::Thread &thread);
	};

	void Gc::Impl::destroy() {
		if (destroyed)
			return;
		destroyed = true;

		{
			util::Lock::L z(threadLock);
			for (ThreadMap::iterator i = threads.begin(); i != threads.end(); ++i) {
				GcImpl::detachThread(i->second.data);
			}
			threads.clear();
		}

		GcImpl::destroy();
	}

	void Gc::Impl::attachThread() {
		os::Thread thread = os::Thread::current();
		util::Lock::L z(threadLock);

		ThreadMap::iterator i = threads.find(thread.id());
		if (i != threads.end()) {
			// Already attached. Increase the refcount.
			i->second.refs++;
		} else {
			// New thread!
			ThreadData d = { 1, GcImpl::attachThread() };
			threads.insert(make_pair(thread.id(), d));
		}
	}

	void Gc::Impl::reattachThread(const os::Thread &thread) {
		util::Lock::L z(threadLock);

		ThreadMap::iterator i = threads.find(thread.id());
		if (i != threads.end()) {
			i->second.refs++;
		} else {
			assert(false, L"Trying to re-attach a new thread!");
		}
	}

	void Gc::Impl::detachThread(const os::Thread &thread) {
		util::Lock::L z(threadLock);

		ThreadMap::iterator i = threads.find(thread.id());
		if (i == threads.end())
			return;

		if (i->second.refs > 1) {
			// Not yet...
			i->second.refs--;
			return;
		}

		GcImpl::detachThread(i->second.data);
		threads.erase(i);
	}

	GcImpl::ThreadData threadData(GcImpl *from, const os::Thread &thread, const GcImpl::ThreadData &def) {
		Gc::Impl *me = (Gc::Impl *)from;
		util::Lock::L z(me->threadLock);

		Gc::Impl::ThreadMap::iterator i = me->threads.find(thread.id());
		if (i == me->threads.end())
			return def;
		else
			return i->second.data;
	}


	/**
	 * Generic interface.
	 */

	Gc::Gc(size_t initialArena, nat finalizationInterval) : impl(new Impl(initialArena, finalizationInterval)) {}

	Gc::~Gc() {
		delete impl;
	}

	void Gc::destroy() {
		impl->destroy();
	}

	MemorySummary Gc::summary() {
		return impl->summary();
	}

	void Gc::collect() {
		impl->collect();
	}

	bool Gc::collect(nat time) {
		return impl->collect(time);
	}

	void Gc::attachThread() {
		impl->attachThread();
	}

	void Gc::reattachThread(const os::Thread &thread) {
		impl->reattachThread(thread);
	}

	void Gc::detachThread(const os::Thread &thread) {
		impl->detachThread(thread);
	}

	void *Gc::alloc(const GcType *type) {
		assert(type->kind == GcType::tType
			|| type->kind == GcType::tFixed
			|| type->kind == GcType::tFixedObj,
			L"Wrong type for calling alloc().");

		return impl->alloc(type);
	}

	void *Gc::allocStatic(const GcType *type) {
		assert(type->kind == GcType::tType
			|| type->kind == GcType::tFixed
			|| type->kind == GcType::tFixedObj,
			L"Wrong type for calling allocStatic().");

		return impl->allocStatic(type);
	}

	GcArray<Byte> *Gc::allocBuffer(size_t count) {
		return impl->allocBuffer(count);
	}

	void *Gc::allocArray(const GcType *type, size_t count) {
		assert(type->kind == GcType::tArray, L"Wrong type for calling allocArray().");

		return impl->allocArray(type, count);
	}

	void *Gc::allocWeakArray(size_t count) {
		return impl->allocWeakArray(&weakArrayType, count);
	}

	Bool Gc::liveObject(RootObject *obj) {
		return Impl::liveObject(obj);
	}

	GcType *Gc::allocType(GcType::Kind kind, Type *type, size_t stride, size_t entries) {
		return impl->allocType(kind, type, stride, entries);
	}

	GcType *Gc::allocType(const GcType *original) {
		GcType *t = allocType(GcType::Kind(original->kind), original->type, original->stride, original->count);
		t->finalizer = original->finalizer;
		for (Nat i = 0; i < original->count; i++) {
			t->offset[i] = original->offset[i];
		}
		return t;
	}

	void Gc::freeType(GcType *type) {
		impl->freeType(type);
	}

	const GcType *Gc::typeOf(const void *mem) {
		return Impl::typeOf(mem);
	}

	void Gc::switchType(void *mem, const GcType *to) {
		assert(typeOf(mem)->stride == to->stride, L"Can not change size of allocations.");
		assert(typeOf(mem)->kind == to->kind, L"Can not change kind of allocations.");

		GcImpl::switchType(mem, to);
	}

	void *Gc::allocCode(size_t code, size_t refs) {
		return impl->allocCode(fmt::wordAlign(code), refs);
	}

	// Get the size of a code allocation.
	size_t Gc::codeSize(const void *alloc) {
		return Impl::codeSize(alloc);
	}

	// Access the metadata of a code allocation. Note: all references must be scannable at *any* time.
	GcCode *Gc::codeRefs(void *alloc) {
		return Impl::codeRefs(alloc);
	}

	Gc::RampAlloc::RampAlloc(Gc &owner) : owner(owner) {
		owner.impl->startRamp();
	}

	Gc::RampAlloc::~RampAlloc() {
		owner.impl->endRamp();
	}

	void Gc::walkObjects(WalkCb fn, void *param) {
		impl->walkObjects(fn, param);
	}

	Gc::Root *Gc::createRoot(void *data, size_t count, bool ambiguous) {
		return (Root *)impl->createRoot(data, count, ambiguous);
	}

	void Gc::destroyRoot(Gc::Root *root) {
		if (root)
			Impl::destroyRoot((Impl::Root *)root);
	}

	GcWatch *Gc::createWatch() {
		return impl->createWatch();
	}

	void Gc::checkMemory() {
		impl->checkMemory();
	}

	void Gc::checkMemory(const void *object, bool recursive) {
		impl->checkMemory(object, recursive);
	}

	void Gc::checkMemoryCollect() {
		impl->checkMemoryCollect();
	}

	void Gc::dbg_dump() {
		impl->dbg_dump();
	}

	const GcLicense *Gc::license() {
		return impl->license();
	}

	const GcType Gc::weakArrayType = {
		GcType::tWeakArray,
		null,
		null,
		sizeof(void *),
		1,
		{}
	};

	/**
	 * Simple linked list to test gc.
	 */
	struct GcLink {
	    nat value;
		GcLink *next;
	};

	static const GcType linkType = {
		GcType::tFixed,
		null,
		null,
		sizeof(GcLink),
		1,
		{ OFFSET_OF(GcLink, next) }
	};

	bool Gc::test(nat times) {
		bool ok = true;

		for (nat j = 0; j < times && ok; j++) {
			GcLink *first = null;
			GcLink *at = null;
			for (nat i = 0; i < 10000; i++) {
				GcLink *l = (GcLink *)alloc(&linkType);
				l->value = i;

				if (at) {
					at->next = l;
					at = l;
				} else {
					first = at = l;
				}
			}

			at = first;
			for (nat i = 0; i < 10000; i++) {
				if (!at) {
					PLN("Premature end at " << i);
					ok = false;
					break;
				}

				if (at->value != i) {
					PLN("Failed: " << at->value << " should be " << i);
					ok = false;
				}

				at = at->next;
			}
			// collect();
		}

		return ok;
	}

}
