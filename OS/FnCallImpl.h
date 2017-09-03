#pragma once
#include "Utils/Platform.h"

namespace storm {
	class Place;
}

namespace os {

	/**
	 * Implementation of the FnCall logic in a generic fashion. Reuqires variadic templates. Included from 'FnCall.h'.
	 */

#ifndef USE_VA_TEMPLATE
#error "The generic FnCall implementation requires varidic templates."
#endif

#if defined(POSIX) && defined(X64)
	// The 'this' parameter is passed as if it was the first parameter of a function. Trying to call
	// a member function only seems to cause trouble!
#define DO_THISCALL(type) (std::is_pointer<type>::value && false) // needs to depend on 'type' to not cause errors.
#else
#error "Please define the 'DO_THISCALL' macro for your compiler here!"
	// This could be needed on some architectures.
#define DO_THISCALL(type) std::is_pointer<type>::value
#endif

	namespace impl {

		template <class Result, class Par, bool member>
		struct ParamHelp {
			template <class ... T>
			static void call(const void *fn, void **params, void *out, nat pos, T& ... args) {
				pos--;
				ParamHelp<Result, typename Par::PrevType, member>
					::call(fn, params, out, pos, *(typename Par::HereType *)params[pos], args...);
			}
		};

		template <class Result>
		struct ParamHelp<Result, Param<void, void>, false> {
			template <class ... T>
			static void call(const void *fn, void **, void *out, nat pos, T& ... args) {
				typedef Result CODECALL (*Fn)(T...);
				Fn p = (Fn)fn;
				new (storm::Place(out)) Result((*p)(args...));
			}
		};

		template <class Result>
		struct ParamHelp<Result, Param<void, void>, true> {
			class Dummy : NoCopy {};

			template <class U, class ... T>
			static typename std::enable_if<DO_THISCALL(U)>::type call(const void *fn, void **, void *out, nat pos, U first, T& ... args) {
				typedef Result CODECALL (Dummy::*Fn)(T...);
				Fn p = null;
				memcpy(&p, &fn, sizeof(void *));
				Dummy *f = (Dummy *)first;
				new (storm::Place(out)) Result((f->*p)(args...));
			}

			template <class U, class ... T>
			static typename std::enable_if<!DO_THISCALL(U)>::type call(const void *fn, void **, void *out, nat pos, U first, T& ... args) {
				typedef Result CODECALL (*Fn)(U, T...);
				Fn p = (Fn)fn;
				new (storm::Place(out)) Result((*p)(first, args...));
			}

			static void call(const void *fn, void **, void *out, nat pos) {
				typedef Result CODECALL (*Fn)();
				Fn p = (Fn)fn;
				new (storm::Place(out)) Result((*p)());
			}
		};

		template <>
		struct ParamHelp<void, Param<void, void>, false> {
			template <class ... T>
			static void call(const void *fn, void **, void *out, nat pos, T& ... args) {
				typedef void CODECALL (*Fn)(T...);
				Fn p = (Fn)fn;
				(*p)(args...);
			}
		};

		template <>
		struct ParamHelp<void, Param<void, void>, true> {
			class Dummy : NoCopy {};

			template <class U, class ... T>
			static typename std::enable_if<DO_THISCALL(U)>::type call(const void *fn, void **, void *out, nat pos, U first, T& ... args) {
				typedef void CODECALL (Dummy::*Fn)(T...);
				Fn p = null;
				memcpy(&p, &fn, sizeof(void *));
				Dummy *f = (Dummy *)first;
				(f->*p)(args...);
			}

			template <class U, class ... T>
			static typename std::enable_if<!DO_THISCALL(U)>::type call(const void *fn, void **, void *out, nat pos, U first, T& ... args) {
				typedef void CODECALL (*Fn)(U, T...);
				Fn p = (Fn)fn;
				(*p)(first, args...);
			}

			static void call(const void *fn, void **, void *out, nat pos) {
				typedef void CODECALL (*Fn)();
				Fn p = (Fn)fn;
				(*p)();
			}
		};

		template <class Result, class Par>
		void call(const void *fn, bool member, void **params, void *first, void *result) {
			if (member) {
				if (first) {
					ParamHelp<Result, Par, true>::call(fn, params, result, Par::count, first);
				} else {
					ParamHelp<Result, Par, true>::call(fn, params, result, Par::count);
				}
			} else {
				if (first) {
					ParamHelp<Result, Par, false>::call(fn, params, result, Par::count, first);
				} else {
					ParamHelp<Result, Par, false>::call(fn, params, result, Par::count);
				}
			}
		}

	}
}
