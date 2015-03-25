#pragma once
#include "Utils/TypeInfo.h"

namespace code {

	/**
	 * Stores function parameters along with a way to manipulate them
	 * in a buffer, so that we can later use them to call functions or
	 * launch new threads. The storage can be backed by either an array
	 * or other memory.
	 *
	 * Since this class stores pointers to values, the user needs to take
	 * care to not pass temporary values to 'param'. For example, the following
	 * is wrong: FnParams().add(1), since 1 will be a temporary variable.
	 */
	class FnParams {
	public:
		// Type declarations.
		typedef void (*CopyFn)(void *, const void *);
		typedef void (*DestroyFn)(void *);

		// Data for a single parameter.
		struct Param {
			CopyFn copy;
			DestroyFn destroy;
			nat size;
			const void *value;
		};

		enum {
			// Capacity from start. If this is large enough, we do not need to
			// allocate more memory while 'add'ing later on!
			initialCapacity = 10,

			// Size of each element stored. Useful when using the 'void *buffer' ctor!
			elemSize = sizeof(Param),
		};

		// Create. Allocates memory automatically.
		FnParams();

		// Create, use the supplied buffer.
		FnParams(void *buffer);

		// Copy.
		FnParams(const FnParams &o);

		// Assign.
		FnParams &operator =(const FnParams &o);

		// Destroy.
		~FnParams();

		// Add a parameter (low-level). Use null, null to indicate that 'value' is the value to be used.
		void add(CopyFn copy, DestroyFn destroy, nat size, const void *value);

		// Add a parameter to the front (low-level).
		void addFirst(CopyFn copy, DestroyFn destroy, nat size, const void *value);

		// Add a parameter (high-level).
		template <class T>
		inline FnParams &add(const T &p) {
			if (!typeInfo<T>().plain()) {
				// Pointer or reference, copy it directly.
				// we know that sizeof(T) == sizeof(void *)
				add(null, null, sizeof(T), *(void **)&p);
			} else {
				add(&FnParams::copy<T>, &FnParams::destroy<T>, sizeof(T), &p);
			}
			return *this;
		}

		// Add a parameter to the front (high-level).
		template <class T>
		inline FnParams &addFirst(const T &p) {
			if (!typeInfo<T>().plain()) {
				// Pointer or reference, copy it directly.
				// we know that sizeof(T) == sizeof(void *)
				addFirst(null, null, sizeof(T), *(void **)&p);
			} else {
				addFirst(&FnParams::copy<T>, &FnParams::destroy<T>, sizeof(T), &p);
			}
			return *this;
		}

		// Number of parameters.
		inline nat count() const { return size; }

		// Total size of parameters.
		nat totalSize() const;

		// Copy parameters to the stack (provide the lowest address, like arrays).
		void copy(void *to) const;

		// Destroy parameters on the stack (provide the lowest address, like arrays).
		void destroy(void *mem) const;

		// Get the size of the class (for machine code generation).
		static Size classSize();

	private:
		// Backing storage.
		Param *params;

		// Number of parameters.
		nat size;

		// Size of the backing storage. If zero, params is assumed to be allocated by
		// someone else, and that it is large enough.
		nat capacity;

		// Resize the buffer.
		void resize(nat to);

		// Copy-ctor invocation.
		template <class T>
		static void copy(void *to, const void *from) {
			const T* f = (const T*)from;
			new (to) T(*f);
		}

		// Destructor invocation.
		template <class T>
		static void destroy(void *obj) {
			T *o = (T *)obj;
			o->~T();
		}
	};

}

#include "FnCall.h"
