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
		typedef void (*Thunk)(const void *fn, void **params, void *result);

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


#ifdef X86

		template <class Result>
		void NAKED execCall(const void *fn, void **params, void **seq, void *result, nat index) {
			__asm {
				mov eax, fn;
				call fn;

				mov ecx, result;
				mov [ecx], eax;

				mov esp, ebp;
				pop ebp;
				ret;
			}
		}

		template <class Par>
		void NAKED pushParam(const void *fn, void **params, void **seq, void *result, nat index) {
			__asm {
				// TODO: Call constructor!
				mov eax, params;
				mov ecx, index;
				dec ecx;
				mov eax, [eax+ecx*4];
				push [eax];

				jmp doCallLoop;
			}
		}

		template <>
		void NAKED pushParam<int>(const void *fn, void **params, void **seq, void *result, nat index) {
			__asm {
				mov eax, params;
				mov ecx, index;
				dec ecx;
				mov eax, [eax+ecx*4];
				push [eax];
				jmp doCallLoop;
			}
		}


		template <class Par>
		void fillSeq(void **seq) {
			seq[Par::count - 1] = &pushParam<Par::HereType>;
			fillSeq<Par::PrevType>(seq);
		}

		template <>
		inline void fillSeq<Param<void, void>>(void **) {}

		// Perform one iteration of the call loop. Elements in 'seq' are assumed to jump back here when they are done.
		inline void NAKED doCallLoop(const void *fn, void **params, void **seq, void *result, nat index) {
			__asm {
				dec index;

				mov eax, seq;
				mov ecx, index;
				mov eax, [eax+ecx*4];

				jmp eax;
			}
		}

		// Initialize the call loop.
		inline void NAKED doCall(const void *fn, void **params, void **seq, void *result, nat index) {
			__asm {
				push ebp;
				mov ebp, esp;

				jmp doCallLoop;
			}
		}

		template <class Result, class Par>
		void call(const void *fn, void **params, void *result) {
			void *seq[Par::count + 1];
			fillSeq<Par>(seq + 1);
			seq[0] = &execCall<Result>;

			doCall(fn, params, seq, result, Par::count + 1);
		}

#endif

	}

	inline impl::Param<void, void> fnCall() {
		return impl::Param<void, void>();
	}

	/**
	 * Storage of the parameters in a type-agnostic way.
	 */
	class FnCall {
	public:
		// Create from pre-computed data (low-level).
		FnCall(void **params, impl::Thunk thunk, BasicTypeInfo result);

		// Destroy.
		~FnCall();

	protected:
		// Dummy ctor.
		FnCall();

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
	class FnCallT : public FnCall {
	public:
		// Create from a 'fnCall' sequence.
		template <class H, class P>
		FnCallT(const impl::Param<H, P> &src) {
			nat count = src.count;
			params(new void *[count], true);
			src.extract(params());

			thunk = &impl::call<T, impl::Param<H, P>>;
			result = typeInfo<T>();
		}

		// Call the function!
		T call(const void *fn) const {
			byte result[sizeof(T)];
			(*thunk)(fn, params(), result);
			T *resPtr = (T *)(void *)result;
			T tmp = *resPtr;
			resPtr->~T();
			return tmp;
		}

	};

}
