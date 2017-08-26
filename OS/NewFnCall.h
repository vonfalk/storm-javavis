#pragma once
#include "Utils/TypeInfo.h"

namespace os {

	/**
	 * Representation of parameters to be passed to a function on another thread.
	 *
	 * Usage:
	 * fnCall<Result>().add(x).add(y)
	 */

	namespace impl {

		// Function declaration used for performing the actual call later on.
		typedef void (*Thunk)(const void *fn, bool member, void **params, void *result);

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
		void call(const void *fn, void **params, void *result);

	}

	inline impl::Param<void, void> fnCall() {
		return impl::Param<void, void>();
	}

	/**
	 * Storage of the parameters in a type-agnostic way.
	 */
	class FnCallRaw {
	public:
		// Create from pre-computed data (low-level).
		FnCallRaw(void **params, impl::Thunk thunk, BasicTypeInfo result);

		// Destroy.
		~FnCallRaw();

	protected:
		// Dummy ctor.
		FnCallRaw();

		// Array containing the source address of the parameters, and a flag indicating wether we're
		// owning the memory or not.
		size_t paramsData;

		// Get/set 'params'.
		void **params() const;
		bool freeParams() const;
		void params(void **data, bool owner);

		// Function to invoke in order to copy parameters and perform the actual call.
		impl::Thunk thunk;

		// Type information about the return type.
		BasicTypeInfo result;
	};

	template <class T>
	class FnCall : public FnCallRaw {
	public:
		// Create from a 'fnCall' sequence.
		template <class H, class P>
		FnCall(const impl::Param<H, P> &src) {
			nat count = src.count;
			params(new void *[count], true);
			src.extract(params());

			thunk = &impl::call<T, impl::Param<H, P>>;
			result = typeInfo<T>();
		}

		// Call the function!
		T call(const void *fn, bool member) const {
			byte result[sizeof(T)];
			(*thunk)(fn, member, params(), result);
			T *resPtr = (T *)(void *)result;
			T tmp = *resPtr;
			resPtr->~T();
			return tmp;
		}

	};

	template <>
	class FnCall<void> : public FnCallRaw {
	public:
		// Create from a 'fnCall' sequence.
		template <class H, class P>
		FnCall(const impl::Param<H, P> &src) {
			nat count = src.count;
			params(new void *[count], true);
			src.extract(params());

			thunk = &impl::call<void, impl::Param<H, P>>;
			result = typeInfo<void>();
		}

		// Call the function!
		void call(const void *fn, bool member) const {
			(*thunk)(fn, member, params(), null);
		}

	};

}

#include "NewFnCallX86.h"
