#pragma once
#include "Utils/TypeInfo.h"

//TODO: Remove this file when Core/Fn is properly ported.

namespace os {

	/**
	 * Stores function parameters along with a way to manipulate them
	 * in a buffer, so that we can later use them to call functions or
	 * launch new threads. The storage can be backed by either an array
	 * or other memory.
	 *
	 * Since this class stores pointers to values, the user needs to take
	 * care to not pass temporary values to 'param'. For example, the following
	 * is wrong: FnParams().add(1), since 1 will be a temporary variable.
	 *
	 * TODO? Use the pre-allocated array to avoid heap allocations alltogether?
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
			const void *value;
			struct {
				nat isFloat : 1;
				nat size : 31;
			};
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
		void add(CopyFn copy, DestroyFn destroy, nat size, bool isFloat, const void *value);

		// Add a parameter to the front (low-level).
		void addFirst(CopyFn copy, DestroyFn destroy, nat size, bool isFloat, const void *value);

		// Add a parameter (high-level).
		// Note: a reference is treated as the value itself.
		// Note: the values used here must persist until after the the call is completed. This is
		// the reason why a const reference is not used; to force non-temporaries as parameters.
		template <class T>
		inline FnParams &add(T &p) {
			TypeInfo i = typeInfo<T>();
			add(&FnParams::copy<T>, &FnParams::destroy<T>, sizeof(T), i.kind == TypeInfo::floatNr, (const void *)&p);
			return *this;
		}

		// Add a parameter to the front (high-level).
		// Note: a reference is treated as the value itself.
		// Note: the values used here must persist until after the the call is completed. This is
		// the reason why a const reference is not used; to force non-temporaries as parameters.
		template <class T>
		inline FnParams &addFirst(T &p) {
			TypeInfo i = typeInfo<T>();
			addFirst(&FnParams::copy<T>, &FnParams::destroy<T>, sizeof(T), i.kind == TypeInfo::floatNr, (const void *)&p);
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
			new (storm::Place(to)) T(*f);
		}

		// Destructor invocation.
		template <class T>
		static void destroy(void *obj) {
			T *o = (T *)obj;
			o->~T();
		}
	};


	/**
	 * FnParams with stack-allocated buffer of a specified size.
	 */
	template <size_t n>
	class FnStackParams : public FnParams {
	public:
		FnStackParams() : FnParams(buffer) {}

	private:
		Param buffer[n];
	};

}

#include "FnCall.h"
