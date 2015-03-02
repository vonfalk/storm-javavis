#pragma once
#include "Utils/TypeInfo.h"

namespace code {

	/**
	 * Call a function with a completely variable parameter list.
	 * You can add parameters once and then call multiple functions
	 * with the same parameters. It goes without saying that this
	 * class neither can nor will check types of the called function.
	 *
	 * When adding parameters, this class only keeps references to
	 * the parameters, which means that you have to keep the object
	 * on the stack until the function call is complete at least.
	 * For example: f.param(String(L"hello")) will not work. Instead
	 * you need to do: String s(L"hello"); f.param(s); Note
	 * that f.param(&foo) will work, even if the pointer is temporary.
	 *
	 * Returning types other than integers and pointers are not yet
	 * supported.
	 */
	class FnCall {
		// Let the UThread use us.
		friend class UThreadData;
	public:

		// Since we want to do param(&p) for a reference, we must take care of
		// that case specifically, since the pointer given to us is temporary!
		template <class T>
		FnCall &param(const T &p) {
			Param par = { &FnCall::copy<T>, &FnCall::destroy<T>, sizeof(T), &p };
			if (!typeInfo<T>().plain()) {
				// if ptr or ref, copy the ptr/ref directly.
				par.copy = &FnCall::copyPtr;
				par.destroy = null;
				// We know that sizeof(T) == sizeof(void*)
				par.value = *(void **)&p;
			}
			params.push_back(par);
			return *this;
		}

		// Since we want to do param(&p) for a reference, we must take care of
		// that case specifically, since the pointer given to us is temporary!
		template <class T>
		FnCall &prependParam(const T &p) {
			Param par = { &FnCall::copy<T>, &FnCall::destroy<T>, sizeof(T), &p };
			if (!typeInfo<T>().plain()) {
				// if ptr or ref, copy the ptr/ref directly.
				par.copy = &FnCall::copyPtr;
				par.destroy = null;
				// We know that sizeof(T) == sizeof(void*)
				par.value = *(void **)&p;
			}
			params.insert(params.begin(), par);
			return *this;
		}

		// Call 'fn' with the parameters previously added. T can be any type except references.
		// Use callRef in that case.
		template <class T>
		T call(const void *fn) {
			// For some reason, we need try-catch around these ones. Otherwise we crash
			// when running in release mode for some reason! Probably, it prevents some inlining
			// somewhere that breaks something...
			try {
				TypeInfo t = typeInfo<T>();
				byte d[sizeof(T)];
				T *data = (T*)d;
				doCall((void *)data, t, fn);
				T copy = *data;
				data->~T();
				return copy;
			} catch (...) {
				throw;
			}
		}

		template <>
		void call(const void *fn) {
			callVoid(fn);
		}

		template <>
		float call(const void *fn) {
			try {
				return doFloatCall(fn);
			} catch (...) {
				throw;
			}
		}

		template <>
		double call(const void *fn) {
			try {
				return doDoubleCall(fn);
			} catch (...) {
				throw;
			}
		}

		// Specific overload for references.
		template <class T>
		T &callRef(const void *fn) {
			try {
				T *ptr = null;
				doCall((void *)&ptr, typeInfo<T &>(), fn);
				return *ptr;
			} catch (...) {
				throw;
			}
		}

		// Parameter count.
		inline nat count() const { return params.size(); }

	private:
		// Data for a single parameter.
		struct Param {
			void (*copy)(const void *, void *);
			void (*destroy)(void *);
			nat size;
			const void *value;
		};

		// All parameters.
		vector<Param> params;

		// Do the call, given type information.
		void doCall(void *result, const TypeInfo &info, const void *fn);

		// Call a function that returns a user-defined type.
		void doUserCall(void *result, nat resultSize, const void *fn);

		// Call a function that returns floating point values.
		float doFloatCall(const void *fn);
		double doDoubleCall(const void *fn);

		void callVoid(const void *fn);

		// Some special cases (for x86 at least)
		void doCall4(void *result, const void *fn);
		void doCall8(void *result, const void *fn);
		void doCallLarge(void *result, nat sz, const void *fn);

		// Find out how much stack space the parameters take.
		nat paramsSize() const;

		// Copy parameters to the stack.
		void copyParams(void *to) const;

		// Destroy parameters on the stack.
		void destroyParams(void *to) const;

		// Copy-ctor invocation.
		template <class T>
		static void copy(const void *from, void *to) {
			const T* f = (const T*)from;
			new (to) T(*f);
		}

		// Copy a raw pointer or reference.
		static inline void copyPtr(const void *from, void *to) {
			const void **t = (const void **)to;
			*t = from;
		}

		// Destructor invocation.
		template <class T>
		static void destroy(void *obj) {
			T *o = (T *)obj;
			o->~T();
		}

	};

}
