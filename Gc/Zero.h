#pragma once

#if STORM_GC == STORM_GC_ZERO
#define STORM_HAS_GC

namespace storm {

	/**
	 * A dummy GC simply allocating memory without ever returning it.
	 */
	class GcImpl {
	public:
		// Create.
		GcImpl(size_t initialArenaSize, Nat finalizationInterval);

		// Destroy. This function is always called, but may be called twice.
		void destroy();

		// Do a full GC now.
		void collect();

		// Spend approx. 'time' ms on a GC. Return 'true' if there is more work to be done.
		Bool collect(Nat time);

		// Type we use to store data with a thread (we don't care).
		typedef Nat ThreadData;

		// Register/deregister a thread with us. The Gc interface handles re-registering for us. It
		// even makes sure that these functions are not called in parallel.
		ThreadData attachThread();
		void detachThread(ThreadData &data);

		// Allocate an object of a specific type.
		void *alloc(const GcType *type);

		// Allocate an object of a specific type in a non-moving pool.
		void *allocStatic(const GcType *type);

		// Allocate a buffer which is not moving nor protected. The memory allocated from here is
		// also safe to access from threads unknown to the garbage collector.
		GcArray<Byte> *allocBuffer(size_t count);

		// Allocate an array of objects.
		void *allocArray(const GcType *type, size_t count);

		// Allocate an array of weak pointers. 'type' is always a GcType instance that is set to
		// WeakArray with one pointer as elements.
		void *allocWeakArray(const GcType *type, size_t count);

		// See if an object is live, ie. not finalized.
		static Bool liveObject(RootObject *obj);

		// Allocate a gc type.
		GcType *allocType(GcType::Kind kind, Type *type, size_t stride, size_t entries);

		// Free a gc type (GC implementations may use garbage collection for these as well).
		void freeType(GcType *type);

		// Get the gc type of an allocation.
		static const GcType *typeOf(const void *mem);

		// Change the gc type of an allocation. Can assume that the new type describes a type of the
		// same size as the old type description.
		static void switchType(void *mem, const GcType *to);

		// Allocate a code block with 'code' bytes of machine code and 'refs' entries of reference data.
		void *allocCode(size_t code, size_t refs);

		// Get the size of a code allocation.
		static size_t codeSize(const void *alloc);

		// Get the metadata of a code allocation.
		static GcCode *codeRefs(void *alloc);

		// Start/end of a ramp allocation. Calls may be nested.
		void startRamp();
		void endRamp();

		// Walk the heap.
		typedef void (*WalkCb)(RootObject *inspect, void *param);
		void walkObjects(WalkCb fn, void *param);

		struct Root;

		// Create a root object.
		Root *createRoot(void *data, size_t count, bool ambiguous);

		// Destroy a root.
		static void destroyRoot(Root *root);

		// Create a watch object (on a GC:d heap, no need to destroy it).
		GcWatch *createWatch();

		// Check memory consistency. Note: Enable checking in 'Gc.cpp' for this to work.
		void checkMemory();
		void checkMemory(const void *object, bool recursive);
		void checkMemoryCollect();

	private:
		// This is a very simple pool allocator for memory acquired from the OS (it is executable).
		Byte *allocStart;
		Byte *allocEnd;
		util::Lock allocLock;

		// Allocate a new pool.
		void newPool();

		// Allocate from the pool.
		void *poolAlloc(size_t bytes);
	};

}

#endif
