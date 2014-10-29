#pragma once
#include "Utils/Templates.h"

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
	 * you need to do: String s(L"hello"); f.param(s);
	 *
	 * Returning types other than integers and pointers are not yet
	 * supported.
	 */
	class FnCall : NoCopy {
	public:

		template <class T>
		FnCall &param(const T &p) {
			Param par = { &FnCall::copy<T>, &FnCall::destroy<T>, &p, sizeof(T) };
			params.push_back(par);
			return *this;
		}

		// Call 'fn' with the parameters previously added. T can be any type except references.
		// Use callRef in that case.
		template <class T>
		T call(void *fn) {
			T *data = (T*)_alloca(sizeof(T));
			if (TypeInfo<T>::pointer() || TypeInfo<T>::reference()) {
				doCall((void *)data, sizeof(T), fn);
			} else {
				doUserCall((void *)data, sizeof(T), fn);
			}
			return *data;
		}

		template <>
		int call(void *fn) {
			int data;
			doCall(&data, sizeof(int), fn);
			return data;
		}

		template <>
		nat call(void *fn) {
			nat data;
			doCall(&data, sizeof(nat), fn);
			return data;
		}

		template <>
		int64 call(void *fn) {
			int64 data;
			doCall(&data, sizeof(int64), fn);
			return data;
		}

		template <>
		nat64 call(void *fn) {
			nat64 data;
			doCall(&data, sizeof(nat64), fn);
			return data;
		}

		template <>
		float call(void *fn) {
			return doFloatCall(fn);
		}

		template <>
		double call(void *fn) {
			return doDoubleCall(fn);
		}

		template <>
		void call(void *fn) {
			callVoid(fn);
		}

		// Specific overload for references.
		template <class T>
		T &callRef(void *fn) {
			T *ptr = null;
			doCall((void *)&ptr, sizeof(ptr), fn);
			return *ptr;
		}

	private:
		// Data for a single parameter.
		struct Param {
			void (*copy)(const void *, void *);
			void (*destroy)(void *);
			const void *value;
			nat size;
		};

		// All parameters.
		vector<Param> params;

		// Do the actual call.
		void doCall(void *result, nat resultSize, void *fn);

		// Call a function that returns a user-defined type.
		void doUserCall(void *result, nat resultSize, void *fn);

		// Call a function that returns floating point values.
		float doFloatCall(void *fn);
		double doDoubleCall(void *fn);

		void callVoid(void *fn);

		// Some special cases (for x86 at least)
		void doCall4(void *result, void *fn);
		void doCall8(void *result, void *fn);
		void doCallLarge(void *result, nat sz, void *fn);


		// Find out how much stack space the parameters take.
		nat paramsSize();

		// Copy parameters to the stack.
		void copyParams(void *to);

		// Destroy parameters on the stack.
		void destroyParams(void *to);

		// Copy-ctor invocation.
		template <class T>
		static void copy(const void *from, void *to) {
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
