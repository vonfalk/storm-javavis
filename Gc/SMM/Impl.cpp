#include "stdafx.h"
#include "Impl.h"

#if STORM_GC == STORM_GC_SMM

#include "Gc/Gc.h"
#include "Gc/Scan.h"
#include "Gc/Format.h"
#include "Thread.h"

namespace storm {

#define KB(n) ((n) * 1024)
#define MB(n) (KB((n) * 1024))

	static const size_t generations[] = {
		KB(128), // Nursery generation
		MB(10), // Intermediate generation
		MB(100), // Persistent generation.
	};

	GcImpl::GcImpl(size_t initialArenaSize, Nat finalizationInterval)
		: arena(initialArenaSize, generations, ARRAY_COUNT(generations)) {}

	void GcImpl::destroy() {
		// TODO: We should destroy everything here already!
	}

	MemorySummary GcImpl::summary() {
		return arena.summary();
	}

	void GcImpl::collect() {
		arena.collect();
	}

	Bool GcImpl::collect(Nat time) {
		return false;
	}

	static THREAD smm::Thread *currThread = null;
	static THREAD GcImpl *currOwner = null;

	GcImpl::ThreadData GcImpl::currentData() {
		smm::Thread *thread = null;

		if (currThread != null && currOwner == this) {
			// We set this up earlier: fast path!
			thread = currThread;
		} else {
			// Either first time allocation, or first time since some other Engine has been
			// used. New setup.
			thread = Gc::threadData(this, os::Thread::current(), null);
			if (!thread)
				throw GcError(L"Trying to allocate memory from a thread not registered with the GC.");

			currThread = thread;
			currOwner = this;
		}

		return thread;
	}

	smm::Allocator &GcImpl::currentAlloc() {
		return currentData()->alloc;
	}

	GcImpl::ThreadData GcImpl::attachThread() {
		return arena.attachThread();
	}

	void GcImpl::detachThread(ThreadData data) {
		arena.detachThread(data);
	}

	void *GcImpl::alloc(const GcType *type) {
		size_t size = fmt::sizeObj(type);
		smm::Allocator &allocator = currentAlloc();
		smm::PendingAlloc alloc;
		void *result;
		do {
			alloc = allocator.reserve(size);
			if (!alloc)
				throw GcError(L"Out of memory (alloc).");
			result = fmt::initObj(alloc.mem(), type, size);
		} while (!alloc.commit());

		return result;
	}

	void *GcImpl::allocStatic(const GcType *type) {
		return null;
	}

	GcArray<Byte> *GcImpl::allocBuffer(size_t count) {
		return null;
	}

	void *GcImpl::allocArray(const GcType *type, size_t count) {
		return null;
	}

	void *GcImpl::allocWeakArray(const GcType *type, size_t count) {
		return null;
	}

	Bool GcImpl::liveObject(RootObject *obj) {
		return obj && !fmt::isFinalized(obj);
	}

	GcType *GcImpl::allocType(GcType::Kind kind, Type *type, size_t stride, size_t entries) {
		size_t size = gcTypeSize(entries) + fmt::headerSize;
		smm::Allocator &allocator = currentAlloc();
		smm::PendingAlloc alloc;
		GcType *result;
		do {
			alloc = allocator.reserve(size);
			if (!alloc)
				throw GcError(L"Out of memory (allocType).");
			result = fmt::initGcType(alloc.mem(), entries);
		} while (!alloc.commit());

		result->kind = kind;
		result->type = type;
		result->stride = stride;
		// Note: entries is set by fmt::initGcType.

		return result;
	}

	void GcImpl::freeType(GcType *type) {}

	const GcType *GcImpl::typeOf(const void *mem) {
		const fmt::Obj *o = fmt::fromClient(mem);
		if (fmt::objIsCode(o))
			return null;
		else
			return &(fmt::objHeader(o)->obj);
	}

	void GcImpl::switchType(void *mem, const GcType *to) {
		fmt::objSetHeader(fmt::fromClient(mem), to);
	}

	void *GcImpl::allocCode(size_t code, size_t refs) {
		return null;
	}

	size_t GcImpl::codeSize(const void *alloc) {
		const fmt::Obj *o = fmt::fromClient(alloc);
		if (fmt::objIsCode(o)) {
			return fmt::objCodeSize(o);
		} else {
			dbg_assert(false, L"Attempting to get the size of a non-code block.");
			return 0;
		}
	}

	GcCode *GcImpl::codeRefs(void *alloc) {
		return fmt::refsCode(fmt::fromClient(alloc));
	}

	void GcImpl::startRamp() {}

	void GcImpl::endRamp() {}

	void GcImpl::walkObjects(WalkCb fn, void *param) {}

	GcImpl::Root *GcImpl::createRoot(void *data, size_t count, bool ambiguous) {
		return null;
	}

	void GcImpl::destroyRoot(Root *root) {}

	GcWatch *GcImpl::createWatch() {
		return null;
	}

	void GcImpl::checkMemory() {
		arena.dbg_verify();
	}

	void GcImpl::checkMemory(const void *object, bool recursive) {}

	void GcImpl::checkMemoryCollect() {}

	void GcImpl::dbg_dump() {
		arena.dbg_dump();
	}

}

#endif
