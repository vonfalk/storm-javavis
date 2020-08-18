#include "stdafx.h"
#include "Impl.h"

#if STORM_GC == STORM_GC_SMM

#include "Gc/Gc.h"
#include "Gc/Scan.h"
#include "Format.h"
#include "Thread.h"
#include "Root.h"
#include "Nonmoving.h"
#include "ArenaTicket.h"
#include "ArenaTicketImpl.h"
#include "History.h"

namespace storm {

#define KB(n) ((n) * 1024)
#define MB(n) (KB((n) * 1024))

	static const size_t generations[] = {
		MB(1), // Nursery generation
		MB(10), // Intermediate generation
		MB(100), // Persistent generation.
	};

	GcImpl::GcImpl(size_t initialArenaSize, Nat finalizationInterval)
		: arena(initialArenaSize, generations, ARRAY_COUNT(generations)) {}

	void GcImpl::destroy() {
		arena.destroy();
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
			if (type->finalizer)
				fmt::setHasFinalizer(result);
		} while (!alloc.commit());

		return result;
	}

	void *GcImpl::allocStatic(const GcType *type) {
		smm::Nonmoving &allocs = arena.nonmoving();
		void *result = arena.lock(allocs, &smm::Nonmoving::alloc, type);
		if (!result)
			throw GcError(L"Out of memory (allocStatic).");

		return result;
	}

	GcArray<Byte> *GcImpl::allocBuffer(size_t count) {
		// TODO: Will most likely work best with a regular 'alloc', but could benefit from a
		// separate implementation without any scanning. 'allocStatic' could work, but the current
		// implementation is not suitable for large amounts of data.

		// TODO: We would like to make sure that this chunk of memory is never protected.
		return (GcArray<Byte> *)allocArray(&byteArrayType, count);
	}

	void *GcImpl::allocArray(const GcType *type, size_t count) {
		size_t size = fmt::sizeArray(type, count);
		smm::Allocator &allocator = currentAlloc();
		smm::PendingAlloc alloc;
		void *result;
		do {
			alloc = allocator.reserve(size);
			if (!alloc)
				throw GcError(L"Out of memory (allocArray).");
			result = fmt::initArray(alloc.mem(), type, size, count);
			if (type->finalizer)
				fmt::setHasFinalizer(result);
		} while (!alloc.commit());

		return result;
	}

	void *GcImpl::allocWeakArray(const GcType *type, size_t count) {
		size_t size = fmt::sizeArray(type, count);
		smm::Allocator &allocator = currentAlloc();
		smm::PendingAlloc alloc;
		void *result;
		do {
			alloc = allocator.reserve(size);
			if (!alloc)
				throw GcError(L"Out of memory (allocWeakArray).");
			result = fmt::initWeakArray(alloc.mem(), type, size, count);
			if (type->finalizer)
				fmt::setHasFinalizer(result);
		} while (!alloc.commit());

		return result;
	}

	Bool GcImpl::liveObject(RootObject *obj) {
		// All objects are destroyed promptly by us, so we don't need this functionality.
		return true;
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

	void GcImpl::freeType(GcType *type) {
		// No need to free type descriptions when using the SMM GC. We will collect them automatically.
	}

	const GcType *GcImpl::typeOf(const void *mem) {
		const fmt::Obj *o = fmt::fromClient(mem);
		if (fmt::objIsCode(o))
			return null;
		else
			return &(fmt::objHeader(o)->obj);
	}

	void GcImpl::switchType(void *mem, const GcType *to) {
		fmt::objReplaceHeader(fmt::fromClient(mem), to);
	}

	void *GcImpl::allocCode(size_t code, size_t refs) {
		size_t size = fmt::sizeCode(code, refs);
		smm::Allocator &allocator = currentAlloc();
		smm::PendingAlloc alloc;
		void *result;
		do {
			alloc = allocator.reserve(size);
			if (!alloc)
				throw GcError(L"Out of memory (allocCode).");
			result = fmt::initCode(alloc.mem(), size, code, refs);
			if (gccode::needFinalization())
				fmt::setHasFinalizer(result);
		} while (!alloc.commit());

		return result;
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

	void GcImpl::startRamp() {
		// Strangely enough, SMM works better without ramp behavior at the moment.
		// arena.startRamp();
	}

	void GcImpl::endRamp() {
		// arena.endRamp();
	}

	class SmmRoot : public GcRoot {
	public:
		smm::Root root;

		// Inexact root?
		bool inexact;

		// The arena.
		smm::Arena &arena;

		SmmRoot(void **data, size_t count, bool inexact, smm::Arena &arena)
			: root(data, count), inexact(inexact), arena(arena) {}
	};

	GcImpl::Root *GcImpl::createRoot(void *data, size_t count, bool inexact) {
		SmmRoot *r = new SmmRoot((void **)data, count, inexact, arena);

		if (inexact) {
			arena.addInexact(r->root);
		} else {
			arena.addExact(r->root);
		}

		return r;
	}

	void GcImpl::destroyRoot(Root *r) {
		SmmRoot *root = (SmmRoot *)r;
		if (root->inexact) {
			root->arena.removeInexact(root->root);
		} else {
			root->arena.removeExact(root->root);
		}
	}

	class SMMWatch : public GcWatch {
	public:
		SMMWatch(GcImpl &impl) : impl(impl), watch(impl.arena.history) {}
		SMMWatch(GcImpl &impl, const smm::AddrWatch &watch) : impl(impl), watch(watch) {}

		virtual void add(const void *addr) {
			watch.add(addr);
		}

		virtual void remove(const void *addr) {
			// Not supported, but we just give false positives, which is OK.
		}

		virtual void clear() {
			watch.clear();
		}

		virtual bool moved() {
			return watch.check();
		}

		virtual bool moved(const void *ptr) {
			return watch.check(ptr);
		}

		virtual bool tagged() {
			return watch.full();
		}

		virtual GcWatch *clone() const {
			return new (impl.alloc(&type)) SMMWatch(impl, watch);
		}

		void set() {
			watch.set();
		}

		static const GcType type;

	private:
		GcImpl &impl;
		smm::AddrWatch watch;
	};

	const GcType SMMWatch::type = {
		GcType::tFixed,
		null,
		null,
		sizeof(SMMWatch),
		0,
		{}
	};

	GcWatch *GcImpl::createWatch() {
		return new (alloc(&SMMWatch::type)) SMMWatch(*this);
	}

	void GcImpl::walk(Walker &context) {
		arena.enter<void, GcImpl, Walker &>(*this, &GcImpl::walkI, context);
	}

	class WalkObjScanner {
	public:
		typedef int Result;
		typedef Walker Source;

		WalkObjScanner(Walker &src) : context(src) {}

		Walker &context;

		ScanOption object(void *start, void *end) {
			if (fmt::objIsCode(fmt::fromClient(start))) {
				context.code(start);
				return scanNone;
			}

			const GcType *type = GcImpl::typeOf(start);
			if (!type)
				return scanNone;

			// Don't show watch allocations to the outside world!
			if (type == &SMMWatch::type) {
				if (context.flags & Walker::fClearWatch) {
					// But make sure they always return 'true' if desired.
					SMMWatch *watch = (SMMWatch *)start;
					watch->set();
				}
				return scanNone;
			}

			switch (type->kind) {
			case GcType::tFixed:
				context.fixed(start);
				break;
			case GcType::tFixedObj:
			case GcType::tType:
				context.object((RootObject *)start);
				break;
			case GcType::tArray:
			case GcType::tWeakArray:
				context.array(start);
				break;
			}

			// We don't need pointer scanning. We're happy with getting to know all objects!
			return scanNone;
		}

		bool fix1(void *ptr) { return true; }
		int fix2(void **ptr) { return 0; }

		bool fixHeader1(GcType *header) { return true; }
		int fixHeader2(GcType **header) { return 0; }
	};

	class WalkExactScanner {
	public:
		typedef int Result;
		typedef Walker Source;

		WalkExactScanner(Walker &src) : context(src) {}

		Walker &context;

		ScanOption object(void *start, void *end) { return scanAll; }

		bool fix1(void *ptr) { return true; }
		int fix2(void **ptr) {
			context.exactPointer(ptr);
			return 0;
		}

		bool fixHeader1(GcType *header) { return true; }
		int fixHeader2(GcType **header) {
			context.exactPointer((void **)header);
			return 0;
		}
	};

	class WalkInexactScanner {
	public:
		typedef int Result;
		typedef Walker Source;

		WalkInexactScanner(Walker &src) : context(src) {}

		Walker &context;

		ScanOption object(void *start, void *end) { return scanAll; }

		bool fix1(void *ptr) { return true; }
		int fix2(void **ptr) {
			context.ambiguousPointer(ptr);
			return 0;
		}

		bool fixHeader1(GcType *header) { return true; }
		int fixHeader2(GcType **header) {
			context.ambiguousPointer((void **)header);
			return 0;
		}
	};

	void GcImpl::walkI(smm::ArenaTicket &ticket, Walker &context) {
		context.prepare();

		if (context.flags & Walker::fObjects) {
			ticket.scanObjects<WalkObjScanner>(context);
		}

		const os::InlineSet<GcRoot> &roots = Gc::allRoots(this);
		if (context.flags & Walker::fExactRoots) {
			for (os::InlineSet<GcRoot>::iterator i = roots.begin(), end = roots.end(); i != end; ++i) {
				SmmRoot *root = (SmmRoot *)*i;
				if (root->inexact)
					continue;

				if (!context.checkRoot(root))
					continue;

				root->root.scan<WalkExactScanner>(context);
			}
		}

		if (context.flags & Walker::fAmbiguousRoots) {
			for (os::InlineSet<GcRoot>::iterator i = roots.begin(), end = roots.end(); i != end; ++i) {
				SmmRoot *root = (SmmRoot *)*i;
				if (!root->inexact)
					continue;

				if (!context.checkRoot(root))
					continue;

				root->root.scan<WalkInexactScanner>(context);
			}

			ticket.scanStacks<WalkInexactScanner>(context);
		}
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
