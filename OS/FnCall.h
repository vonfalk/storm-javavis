#pragma once
#include "Utils/TypeInfo.h"

namespace os {

	/**
	 * Representation of parameters to be passed to a function on another thread.
	 *
	 * Usage:
	 * fnCall<Result>().add(x).add(y)
	 */

	// Function declaration used for performing the actual call later on.
	typedef void (*CallThunk)(const void *fn, bool member, void **params, void *first, void *result);

	namespace impl {

		// Storing a sequence of parameters.
		template <class Here, class Prev>
		struct Param {
			typedef Prev PrevType;
			typedef Here HereType;
			enum { count = Prev::count + 1 };

			Prev prev;
			Here *param;

			Param(const Prev &prev, Here &param) : prev(prev), param(&param) {}

			template <class T>
			Param<T, Param<Here, Prev>> add(T &param) const {
				return Param<T, Param<Here, Prev>>(*this, param);
			}

			void extract(void **into) const {
				into[count - 1] = param;
				prev.extract(into);
			}
		};

		template <>
		struct Param<void, void> {
			typedef void PrevType;
			enum { count = 0 };

			Param() {}

			template <class T>
			Param<T, Param<void, void>> add(T &param) const {
				return Param<T, Param<void, void>>(*this, param);
			}

			void extract(void **) const {}
		};


		template <class Result, class Par>
		void call(const void *fn, bool member, void **params, void *first, void *result);

	}

	inline impl::Param<void, void> fnCall() {
		return impl::Param<void, void>();
	}

	/**
	 * Storage of the parameters in a type-agnostic way.
	 */
	class FnCallRaw {
		friend class UThread;
	public:
		// Create from pre-computed data (low-level).
		FnCallRaw(void **params, CallThunk thunk);

		// Destroy.
		~FnCallRaw();

		// Helper to invoke 'thunk'.
		inline void callRaw(const void *fn, bool member, void *first, void *result) const {
			(*thunk)(fn, member, params(), first, result);
		}

	protected:
		// Dummy ctor.
		FnCallRaw();

		// Array containing the source address of the parameters, and a flag indicating wether we're
		// owning the memory or not.
		size_t paramsData;

		// Set 'params'.
		bool freeParams() const;
		void params(void **data, bool owner);

		// Get 'params'.
		void **params() const;

		// Function to invoke in order to copy parameters and perform the actual call.
		CallThunk thunk;

#ifdef USE_MOVE
		FnCallRaw(FnCallRaw &&o);
		FnCallRaw &operator =(FnCallRaw &&o);
#endif

	private:
		// Copy and assignment are not supported.
		FnCallRaw(const FnCallRaw &o);
		FnCallRaw &operator =(const FnCallRaw &o);
	};

	template <class T, int alloc = -1>
	class FnCall : public FnCallRaw {
	public:
		// Create from a 'fnCall' sequence.
		template <class H, class P>
		FnCall(const impl::Param<H, P> &src) {
			nat count = src.count;
			if (alloc < 0) {
				params(new void *[count], true);
			} else {
				assert(alloc >= count, L"Not enough memory allocated to FnCall.");
				params(memory, false);
			}
			src.extract(params());

			thunk = &impl::call<T, impl::Param<H, P>>;
		}

		// Call the function, optionally adding a first parameter.
		T call(const void *fn, bool member, void *first = null) const {
			byte result[sizeof(T)];
			(*thunk)(fn, member, params(), first, result);
			T *resPtr = (T *)(void *)result;
			T tmp = *resPtr;
			resPtr->~T();
			return tmp;
		}

	private:
		void *memory[alloc > 0 ? alloc : 1];
	};

	template <int alloc>
	class FnCall<void, alloc> : public FnCallRaw {
	public:
		// Create from a 'fnCall' sequence.
		template <class H, class P>
		FnCall(const impl::Param<H, P> &src) {
			nat count = src.count;
			if (alloc < 0) {
				params(new void *[count], true);
			} else {
				assert(alloc >= count, L"Not enough memory allocated to FnCall.");
				params(memory, false);
			}
			src.extract(params());

			thunk = &impl::call<void, impl::Param<H, P>>;
		}

		// Call the function, optionally adding a first parameter.
		void call(const void *fn, bool member, void *first = null) const {
			(*thunk)(fn, member, params(), first, null);
		}

	private:
		void *memory[alloc > 0 ? alloc : 1];
	};

}

#if defined(VISUAL_STUDIO) && defined(X86)
#include "FnCallX86.h"
#else
#include "FnCallImpl.h"
#endif
