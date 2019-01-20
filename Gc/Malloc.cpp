#include "stdafx.h"
#include "Malloc.h"

#ifdef STORM_GC_MALLOC

namespace storm {

	// Word-align something.
	static inline size_t align(size_t data) {
		return (data + sizeof(size_t) - 1) & ~(sizeof(size_t) - 1);
	}

	static const size_t headerSizeWords = 4;

	// Very primitive pool allocator without support for freeing memory.
	static byte *allocStart = null;
	static byte *allocEnd = null;
	static const size_t allocChunk = 1024*1024; // 1MB at a time.
	static util::Lock allocLock;

#if defined(WINDOWS)

	static void newPool() {
		allocStart = (byte *)VirtualAlloc(null, allocChunk, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
		allocEnd = allocStart + allocChunk;
	}

#elif defined(POSIX)

	static void newPool() {
		allocStart = (byte *)mmap(null, allocChunk, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		allocEnd = allocStart + allocChunk;
	}

#endif

	static void *poolAlloc(size_t bytes) {
		util::Lock::L z(allocLock);

		if (size_t(allocEnd - allocStart) < bytes)
			newPool();

		void *result = allocStart;
		allocStart += bytes;
		return result;
	}

	Gc::Gc(size_t initial, nat finalizationInterval) : finalizationInterval(finalizationInterval) {}

	Gc::~Gc() {}

	void Gc::destroy() {}

	void Gc::collect() {}

	bool Gc::collect(nat time) {
		return false;
	}

	void Gc::attachThread() {}

	void Gc::reattachThread(const os::Thread &thread) {}

	void Gc::detachThread(const os::Thread &thread) {}

	void *Gc::alloc(const GcType *type) {
		size_t size = align(type->stride) + headerSizeWords*sizeof(size_t);
		void *mem = poolAlloc(size);
		memset(mem, 0, size);
		memset(mem, 0xFF, headerSizeWords*sizeof(size_t));

		*(const GcType **)mem = type;
		void *start = (size_t *)mem + headerSizeWords;
		assert(typeOf(start) == type);
		return start;
	}

	void *Gc::allocStatic(const GcType *type) {
		return alloc(type);
	}

	GcArray<Byte> *Gc::allocBuffer(size_t count) {
		return (GcArray<Byte> *)allocArray(&byteArrayType, count);
	}

	void *Gc::allocArray(const GcType *type, size_t count) {
		size_t size = align(type->stride)*count + 2*sizeof(size_t) + headerSizeWords*sizeof(size_t);
		void *mem = poolAlloc(size);
		memset(mem, 0, size);
		memset(mem, 0xFF, headerSizeWords*sizeof(size_t));

		*(const GcType **)mem = type;

		void *start = (size_t *)mem + headerSizeWords;
		*(size_t *)start = count;
		return start;
	}

	void *Gc::allocWeakArray(size_t count) {
		size_t size = sizeof(void*)*count + 2*sizeof(size_t) + headerSizeWords*sizeof(size_t);
		void *mem = poolAlloc(size);
		memset(mem, 0, size);
		memset(mem, 0xFF, headerSizeWords*sizeof(size_t));

		*(const GcType **)mem = &weakArrayType;

		void *start = (size_t *)mem + headerSizeWords;
		*(size_t *)start = (count << 1) | 1;
		return start;
	}

	Gc::RampAlloc::RampAlloc(Gc &owner) : owner(owner) {}

	Gc::RampAlloc::~RampAlloc() {}

	GcType *Gc::allocType(GcType::Kind kind, Type *type, size_t stride, size_t entries) {
		size_t s = gcTypeSize(entries);
		GcType *t = (GcType *)malloc(s);
		memset(t, 0, s);
		t->kind = kind;
		t->type = type;
		t->stride = stride;
		t->count = entries;
		return t;
	}

	GcType *Gc::allocType(const GcType *src) {
		size_t s = gcTypeSize(src->count);
		GcType *t = (GcType *)malloc(s);
		memcpy(t, src, s);
		return t;
	}

	void Gc::freeType(GcType *type) {
		free(type);
	}

	const GcType *Gc::typeOf(const void *mem) {
		const GcType **data = (const GcType **)mem;

		// for (nat i = 0; i < (headerSizeWords-1)*sizeof(size_t); i++) {
		// 	byte *addr = (byte *)data - i - 1;
		// 	if (*addr != 0xFF) {
		// 		PLN(L"Wrote before the object: " << mem << L" offset: -" << (i+1));
		// 	}
		// }

		return *(data - headerSizeWords);
	}

	void Gc::switchType(void *mem, const GcType *to) {
		const GcType **data = (const GcType **)mem;
		*(data - headerSizeWords) = to;
	}

	void *Gc::allocCode(size_t code, size_t refs) {
		// static memory::Manager mgr;

		code = align(code);
		size_t size = code + sizeof(GcCode) + refs*sizeof(GcCodeRef) - sizeof(GcCodeRef) + headerSizeWords*sizeof(size_t);
		void *mem = poolAlloc(size);
		// void *mem = mgr.allocate(size);
		memset(mem, 0, size);
		memset((size_t *)mem + 1, 0xFF, (headerSizeWords - 1)*sizeof(size_t));

		*(size_t *)mem = code;

		void *start = (void **)mem + headerSizeWords;
		void *refPtr = (byte *)start + code;
		*(size_t *)refPtr = refs;

		return start;
	}

	size_t Gc::codeSize(const void *alloc) {
		const size_t *d = (const size_t *)alloc;
		return *(d - headerSizeWords);
	}

	GcCode *Gc::codeRefs(void *alloc) {
		void *p = ((byte *)alloc) + align(codeSize(alloc));
		return (GcCode *)p;
	}

	void Gc::walkObjects(WalkCb fn, void *param) {
		// Nothing to do...
	}

	bool Gc::isCodeAlloc(void *ptr) {
		// Maybe... We do not know.
		return true;
	}

	Gc::Root *Gc::createRoot(void *data, size_t count) {
		// No roots here!
		return null;
	}

	Gc::Root *Gc::createRoot(void *data, size_t count, bool ambiguous) {
		// No roots here!
		return null;
	}

	void Gc::destroyRoot(Root *root) {}

	class MallocWatch : public GcWatch {
	public:
		virtual void add(const void *addr) {}
		virtual void remove(const void *addr) {}
		virtual void clear() {}
		virtual bool moved() { return false; }
		virtual bool moved(const void *addr) { return false; }
		virtual GcWatch *clone() const {
			return new MallocWatch();
		}
	};

	GcWatch *Gc::createWatch() {
		return new MallocWatch();
	}

	void Gc::checkMemory() {}

	void Gc::checkMemory(const void *object) {
		checkMemory(object, true);
	}

	void Gc::checkMemory(const void *object, bool recursive) {
		const GcType **data = (const GcType **)object;

		for (nat i = 0; i < (headerSizeWords-1)*sizeof(size_t); i++) {
			byte *addr = (byte *)data - i - 1;
			if (*addr != 0xFF) {
				PLN(L"Wrote before the object: " << object << L" offset: -" << (i+1));
			}
		}
	}

	void Gc::checkMemoryCollect() {}

}

#endif
