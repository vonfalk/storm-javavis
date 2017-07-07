#pragma once



namespace os {

	/**
	 * Implementation of the FnCall logic in a generic fashion. Reuqires variadic templates. Included from 'FnCall.h'.
	 */

#ifndef USE_VA_TEMPLATE
#error "The generic FnCall implementation requires varidic templates."
#endif

	namespace impl {

		template <class Result, class Par, bool member>
		struct ParamHelp {
			template <class ... T>
			static void call(const void *fn, void **params, void *out, nat pos, T& ... args) {
				ParamHelp<Result, typename Par::PrevType, member>
					::call(fn, params, out, pos, args..., *(typename Par::HereType *)params[pos]);
			}
		};

		template <class Result>
		struct ParamHelp<Result, Param<void, void>, false> {
			template <class ... T>
			static void call(const void *fn, void **, void *out, nat pos, T& ... args) {
				typedef Result CODECALL (*Fn)(T...);
				Fn p = (Fn)fn;
				Result *r = (Result *)out;
				*r = (*p)(args...);
			}
		};

		template <class Result>
		struct ParamHelp<Result, Param<void, void>, true> {
			class Dummy : NoCopy {};

			template <class U, class ... T>
			static typename std::enable_if<std::is_pointer<U>::value>::type call(const void *fn, void **, void *out, nat pos, U first, T& ... args) {
				typedef Result CODECALL (Dummy::*Fn)(T...);
				Fn p = null;
				memcpy(&p, &fn, sizeof(void *));
				Dummy *f = (Dummy *)first;
				Result *r = (Result *)out;
				*r = (f->*p)(args...);
			}

			template <class U, class ... T>
			static typename std::enable_if<!std::is_pointer<U>::value>::type call(const void *fn, void **, void *out, nat pos, U first, T& ... args) {
				typedef Result CODECALL (*Fn)(U, T...);
				Fn p = (Fn)fn;
				Result *r = (Result *)out;
				*r = (*p)(first, args...);
			}

			static void call(const void *fn, void **, void *out, nat pos) {
				typedef Result CODECALL (*Fn)();
				Fn p = (Fn)fn;
				Result *r = (Result *)out;
				*r = (*p)();
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
			static typename std::enable_if<std::is_pointer<U>::value>::type call(const void *fn, void **, void *out, nat pos, U first, T& ... args) {
				typedef void CODECALL (Dummy::*Fn)(T...);
				Fn p = null;
				memcpy(&p, &fn, sizeof(void *));
				Dummy *f = (Dummy *)first;
				(f->*p)(args...);
			}

			template <class U, class ... T>
			static typename std::enable_if<!std::is_pointer<U>::value>::type call(const void *fn, void **, void *out, nat pos, U first, T& ... args) {
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
					ParamHelp<Result, Par, true>::call(fn, params, result, 0, first);
				} else {
					ParamHelp<Result, Par, true>::call(fn, params, result, 0);
				}
			} else {
				if (first) {
					ParamHelp<Result, Par, false>::call(fn, params, result, 0, first);
				} else {
					ParamHelp<Result, Par, false>::call(fn, params, result, 0);
				}
			}
		}

	}
}
