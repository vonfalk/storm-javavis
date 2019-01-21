#include "stdafx.h"
#include "Gc.h"
#include "Utils/Memory.h"

namespace storm {

	Gc::Gc(size_t initialArena, nat finalizationInterval) : impl(initialArena, finalizationInterval) {}

	Gc::~Gc() {
		destroy();
	}

	void Gc::destroy() {
		{
			util::Lock::L z(threadLock);
			for (ThreadMap::iterator i = threads.begin(); i != threads.end(); ++i) {
				impl.detachThread(i->second.data);
			}
			threads.clear();
		}

		impl.destroy();
	}

	void Gc::collect() {
		impl.collect();
	}

	bool Gc::collect(nat time) {
		return impl.collect(time);
	}

	void Gc::attachThread() {
		os::Thread thread = os::Thread::current();
		util::Lock::L z(threadLock);

		ThreadMap::iterator i = threads.find(thread.id());
		if (i != threads.end()) {
			// Already attached. Increase the refcount.
			i->second.refs++;
		} else {
			// New thread!
			ThreadData d = { 1, impl.attachThread() };
			threads.insert(make_pair(thread.id(), d));
		}
	}

	void Gc::reattachThread(const os::Thread &thread) {
		util::Lock::L z(threadLock);

		ThreadMap::iterator i = threads.find(thread.id());
		if (i != threads.end()) {
			i->second.refs++;
		} else {
			assert(false, L"Trying to re-attach a new thread!");
		}
	}

	void Gc::detachThread(const os::Thread &thread) {
		util::Lock::L z(threadLock);

		ThreadMap::iterator i = threads.find(thread.id());
		if (i == threads.end())
			return;

		if (i->second.refs > 1) {
			// Not yet...
			i->second.refs--;
			return;
		}

		impl.detachThread(i->second.data);
		threads.erase(i);
	}


	GcType *Gc::allocType(GcType::Kind kind, Type *type, size_t stride, size_t entries) {
		return impl.allocType(kind, type, stride, entries);
	}

	GcType *Gc::allocType(const GcType *original) {
		GcType *t = allocType(GcType::Kind(original->kind), original->type, original->stride, original->count);
		for (Nat i = 0; i < original->count; i++) {
			t->offset[i] = original->offset[i];
		}
		return t;
	}

	void Gc::freeType(GcType *type) {
		impl.freeType(type);
	}

	void Gc::switchType(void *mem, const GcType *to) {
		assert(typeOf(mem)->stride == to->stride, L"Can not change size of allocations.");
		assert(typeOf(mem)->kind == to->kind, L"Can not change kind of allocations.");

		GcImpl::switchType(mem, to);
	}

	Gc::RampAlloc::RampAlloc(Gc &owner) : owner(owner) {
		util::Lock::L z(owner.rampLock);
		if (owner.rampDepth++ == 0)
			owner.impl.startRamp();
	}

	Gc::RampAlloc::~RampAlloc() {
		util::Lock::L z(owner.rampLock);
		if (owner.rampDepth-- == 0)
			owner.impl.endRamp();
	}

	void Gc::walkObjects(WalkCb fn, void *param) {
		impl.walkObjects(fn, param);
	}

	void Gc::checkMemory() {
		impl.checkMemory();
	}

	void Gc::checkMemory(const void *object, bool recursive) {
		impl.checkMemory(object, recursive);
	}

	void Gc::checkMemoryCollect() {
		impl.checkMemoryCollect();
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
