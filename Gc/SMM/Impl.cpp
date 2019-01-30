#include "stdafx.h"
#include "Impl.h"

#if STORM_GC == STORM_GC_SMM

#include "Gc/Scan.h"
#include "Gc/Format.h"

namespace storm {

	GcImpl::GcImpl(size_t initialArenaSize, Nat finalizationInterval) : arena(initialArenaSize) {}

	void GcImpl::destroy() {}

	void GcImpl::collect() {}

	Bool GcImpl::collect(Nat time) {
		return false;
	}

	GcImpl::ThreadData GcImpl::attachThread() {
		return null;
	}

	void GcImpl::detachThread(ThreadData data) {}

	void *GcImpl::alloc(const GcType *type) {
		return null;
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
		return null;
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

	void GcImpl::checkMemory() {}

	void GcImpl::checkMemory(const void *object, bool recursive) {}

	void GcImpl::checkMemoryCollect() {}

}

#endif
