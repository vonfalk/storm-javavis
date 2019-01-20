#pragma once
#include "Utils/Exception.h"
#include "Utils/Lock.h"

#include "Core/GcType.h"
#include "Core/GcArray.h"
#include "Core/GcWatch.h"
#include "Core/GcCode.h"

/**
 * Include all possible GC implementations. Only one will be selected.
 */
#include "Mps.h"
#include "Malloc.h"

#ifndef STORM_HAS_GC
// If this happens, an unselected GC has been selected in Core/Storm.h.
#error "No GC selected!"
#endif

namespace storm {

	class Type;

	/**
	 * This is the interface to the garbage collector. To reliably run a GC, we need to be able to
	 * describe where each type stores its pointers, and possibly what finalizers to execute.
	 *
	 * Storm supports multiple garbage collectors. See 'Core/Storm.h' for details on how to select
	 * between them.
	 *
	 * Note: Any types describing data layout should be allocated from the corresponding Gc class,
	 * as special care might need to be taken not to interfere with the garbage collector.
	 */
	class Gc : NoCopy {
	public:
		// Create.
		// 'initialArena' - an initial estimate of the arena size. May be disregarded by the gc if needed.
		// 'finalizationInterval' - how seldom the gc should check for finalizations. An interval of 500 means
		//                          every 500 allocations.
		Gc(size_t initialArena, nat finalizationInterval);

		// Destroy.
		~Gc();

		// Destroy the Gc before the destructor is executed.
		void destroy();


		/**
		 * Manual garbage collection hints.
		 */

		// Do a full garbage collection now.
		void collect();

		// Spend approx 'time' ms on an incremental collection if possible. Returns true if there is more to do.
		bool collect(nat time);

		// TODO: Add interface for managing pause times and getting information about allocations.


		/**
		 * Thread management.
		 */

		// Register the current thread for use with the gc.
		void attachThread();

		// Re-register a thread for use with the gc. Can not be used for new threads.
		void reattachThread(const os::Thread &thread);

		// Unregister a thread from the gc. This has to be done before the thread is destroyed.
		void detachThread(const os::Thread &thread);


		/**
		 * Memory allocation.
		 */

		// Allocate an object of a specific type. Assumes type->type == tFixed.
		void *alloc(const GcType *type);

		// Allocate an object of a specific type in a non-moving pool.
		void *allocStatic(const GcType *type);

		// Allocate a buffer which is not moving nor protected. The memory allocated from here is
		// also safe to access from threads unknown to the garbage collector.
		GcArray<byte> *allocBuffer(size_t count);

		// Allocate an array of objects. Assumes type->type == tArray.
		void *allocArray(const GcType *type, size_t count);

		// Allocate an array of weak pointers.
		void *allocWeakArray(size_t count);

		// See if the object is live. An object is considered live until it has been
		// finalized. Finalized objects may not be collected immediately after they have been
		// finalized, and therefore they may still appear inside weak sets etc. after that. The GC
		// implementation marks such objects, so that it is possible to see if they are finalized.
		static bool liveObject(RootObject *obj);


		/**
		 * Notify the GC that we're trying to allocate a fairly large data structure from this
		 * thread in the near future, most of which will be dead right after the work is
		 * complete. This hint could be ignored by the underlying implementation.
		 */
		class RampAlloc {
		public:
			RampAlloc(Gc &owner);
			~RampAlloc();

		private:
			RampAlloc(const RampAlloc &);
			RampAlloc &operator =(const RampAlloc &);

			Gc &owner;
		};


		/**
		 * Management of Gc types.
		 *
		 * These objects are semi-managed by the GC. They are not collected automatically, but they
		 * are destroyed together with the GC object. That means, small leaks of GcType objects are
		 * not a problem as long as they don't grow over time.
		 */

		// Allocate a gc type.
		GcType *allocType(GcType::Kind kind, Type *type, size_t stride, size_t entries);
		GcType *allocType(const GcType *original);

		// Deallocate a gc type.
		void freeType(GcType *type);

		// Get the gc type of an allocation.
		static const GcType *typeOf(const void *mem);

		// Change the gc type of an allocation. Used during startup to change header from a fake one
		// (to allow basic scanning) to a real one (with a proper Type) when startup has progressed
		// further.
		static void switchType(void *mem, const GcType *to);


		/**
		 * Allocation of code storage. Code allocations are slightly more complex than regular data
		 * allocations. First and foremost, this memory needs to be executable, but it also contains
		 * references to other objects in rather weird formats (for example, relative
		 * offsets). Aside from that, there is also the requirement that we may not have a header on
		 * these objects, even though they are generally different size (so we can not use GcArray,
		 * which stores its size at offset 0).
		 *
		 * To meet these requirements, a code allocation consist of two parts: one code part and one
		 * metadata part. The code part is first, and at some later point (the gc keeps track of
		 * this), the metadata block begins. Here, information about all references in the current
		 * block are stored.
		 */

		// Allocate a code block with 'code' bytes of machine code storage and 'refs' entries of reference data.
		void *allocCode(size_t code, size_t refs);

		// Get the size of a code allocation.
		static size_t codeSize(const void *alloc);

		// Access the metadata of a code allocation. Note: all references must be scannable at *any* time.
		static GcCode *codeRefs(void *alloc);

		// Is this pointer allocated by the GC? May return false positives.
		bool isCodeAlloc(void *ptr);


		/**
		 * Iterate through all objects on the heap.
		 *
		 * Note: inside the callback it is only possible to access the object which is passed to the
		 * function and any memory on the stack (this means, for example, it is not possible to use
		 * as<> or call any virtual members in the object). Do not take locks nor attempt to perform
		 * a thread switch inside the callback.
		 */

		// Callback function for a heap walk.
		typedef void (*WalkCb)(RootObject *inspect, void *param);

		// Walk the heap. This usually incurs a full Gc, so it is not a cheap operation.
		void walkObjects(WalkCb fn, void *param);


		/**
		 * Roots.
		 */

		struct Root;

		// Allocate a root that scans an array of pointers.
		Root *createRoot(void *data, size_t count);
		Root *createRoot(void *data, size_t count, bool ambiguous);

		// Destroy a root.
		static void destroyRoot(Root *root);


		/**
		 * Watch object.
		 */

		GcWatch *createWatch();


		/**
		 * Debugging/testing.
		 */

		// Stress-test the gc by allocating a large number of objects.
		bool test(nat times = 100);

		// Check memory consistency. Note: Enable checking in 'Gc.cpp' for this to work.
		void checkMemory();

		// Check consistency of a single object. Note: Enable checking in 'Gc.cpp' for this to work.
		void checkMemory(const void *object);
		void checkMemory(const void *object, bool recursive);

		// Do a gc and check memory collection (sometimes forces memory issues to appear better than
		// just calling 'checkMemory').
		void checkMemoryCollect();

	private:
		// GcType for weak arrays.
		static const GcType weakArrayType;

		// Initial arena size.
		size_t initialArena;

		// Finalization interval.
		nat finalizationInterval;

		// GC-specific things. Defined in the header file corresponding to the GC:s entry point.
		// Called with a reference to this object, so that it can be initialized properly.
		GcData data;

		friend class GcData;
	};


	/**
	 * Errors thrown when interacting with the GC.
	 */
	class GcError : public Exception {
	public:
		GcError(const String &msg) : msg(msg) {}
		virtual String what() const { return msg; }
	private:
		String msg;
	};


}
