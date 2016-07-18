#pragma once
#include "Utils/Exception.h"
#include "Utils/Lock.h"
#include "OS/Thread.h"

#include "gc/mps.h"

#include "GcArray.h"

namespace storm {

	class Type;

	/**
	 * This file contains the interface to the garbage collector. To reliably run a GC, we need to
	 * be able to describe where each type hides its pointers, and possibly what finalizers to
	 * run.
	 *
	 * Note: Any types describing data layout should be allocated from the corresponding Gc class,
	 * as special care might need to be taken not to interfere with the garbage collector.
	 *
	 * TODO: Revise the sizes of data types used on 64-bit systems. Some sizes and offsets can be
	 * stored as 32-bit numbers instead of 64-bit numbers to save space.
	 */


	/**
	 * Description of a type from the view of the garbage collector. In general, we maintain one
	 * GcType for each Type in the system. A GcType is different from a Type as it only describes
	 * the part of the memory layout relevant to the garbage collector (ie. pointer offsets). It
	 * also does this in a flattened format, so that scanning is fast.
	 *
	 * Make sure to allocate these through the garbage collector interface, as these might need to
	 * be kept separate from other data to ensure the garbage collector will work properly.
	 */
	struct GcType {
		// Possible variations in the description.
		enum Kind {
			// A type that has a fixed layout and size (ie. classes, structs).
			tFixed,

			// A variant of tFixed, where offset[0] points to a GcType to be scanned. Used by the
			// Type type to properly scan its GcType member.
			tType,

			// Repeated occurence of another type (ie. an array). The number of repetitions is stored as
			// a size_t in the first element of the allocation (to keep alignment), followed by a number
			// of repeated fixed size allocations. Use GcArray<T> for convenient access.
			tArray,
		};

		// Type. size_t since we want to know the size properly. Has to be first.
		size_t kind;

		/**
		 * Other useful data for the rest of the system:
		 */

		// (scanned) reference to the full description of this type. NOTE: Do not change this after
		// the GcType has been created. Doing so may confuse the GC, as this is an unsupported
		// remote reference. Not declared const due to how we are using it.
		Type *type;

		/**
		 * Description of pointer offsets:
		 */

		// Number of bytes to skip for each element. In the case of tFixed, the size of the allocation.
		size_t stride;

		// Number of offsets here.
		size_t count;

		// Pointer offsets within each element.
		size_t offset[1];
	};

	/**
	 * Internal description of thread-local data for the garbage collector.
	 */
	struct GcThread;

	/**
	 * Interface to the garbage collector. Storm supports multiple garbage collectors, see Storm.h
	 * on how to choose between them. An instance of this class represents an isolated gc arena,
	 * from which we can allocate memory.
	 */
	class Gc : NoCopy {
	public:
		// Create, gives an initial arena size. This is only an estimate that may be disregarded by
		// the gc.
		Gc(size_t initialArena);

		// Destroy.
		~Gc();

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

		// Allocate an array of objects. Assumes type->type == tArray.
		void *allocArray(const GcType *type, size_t count);

		template <class T>
		GcArray<T> *allocArray(const GcType *type, size_t count) {
			return (GcArray<T> *)allocArray(type, count);
		}

		/**
		 * Management of Gc types.
		 */

		// Allocate a gc type.
		GcType *allocType(GcType::Kind kind, Type *type, size_t stride, size_t entries);

		// Deallocate a gc type.
		void freeType(GcType *type);

		// Get the gc type of an allocation.
		static const GcType *allocType(const void *mem);

		// Change the gc type of an allocation. Used during startup to change header from a fake one
		// (to allow basic scanning) to a real one (with a proper Type) when startup has progressed
		// further.
		static void switchType(void *mem, const GcType *to);

		/**
		 * Roots.
		 */

		struct Root;

		// Allocate a root that scans an array of pointers.
		Root *createRoot(void *data, size_t count);

		// Destroy a root.
		void destroyRoot(Root *root);

		/**
		 * Debugging/testing.
		 */

		// Stress-test the gc by allocating a large number of objects.
		void test();

	private:
		// Internal variables which are implementation-specific:
#ifdef STORM_GC_MPS
		// Current arena, pool and format.
		mps_arena_t arena;
		mps_pool_t pool;
		mps_fmt_t format;
		mps_chain_t chain;

		// Separate non-moving pool for storing Type-objects. Note: we only have one allocation
		// point for the types since they are rarely allocated.
		mps_pool_t typePool;
		mps_ap_t typeAllocPoint;
		util::Lock typeAllocLock;

		// Description of all attached threads.
		typedef map<uintptr_t, GcThread *> ThreadMap;
		ThreadMap threads;

		// Lock for manipulating the attached threads.
		util::Lock threadLock;

		// Allocate an object in the Type pool.
		void *allocType(const GcType *type);

		// Attach/detach thread. Sets up/tears down all members in Thread, nothing else.
		void attach(GcThread *thread, const os::Thread &oThread);
		void detach(GcThread *thread);

		// Find the allocation point for the current thread.
		mps_ap_t &currentAllocPoint();
#endif
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
