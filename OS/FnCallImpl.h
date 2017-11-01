#pragma once
#include "Utils/Platform.h"

namespace storm {
	class Place;
}

namespace os {

	/**
	 * Implementation of the FnCall logic in a generic fashion. Reuqires variadic templates. Included from 'FnCall.h'.
	 */

	namespace impl {


#ifndef USE_VA_TEMPLATE
#error "The generic FnCall implementation requires varidic templates."
#endif

		// Helper macro, enabling the function (with void return value) if 'U' is a pointer.
#define IF_POINTER(U) typename std::enable_if<std::is_pointer<U>::value>::type
#define IF_NOT_POINTER(U) typename std::enable_if<!std::is_pointer<U>::value>::type

		/**
		 * Forward declarations of platform specific alterations to the generic behavior.
		 */

		// Copy a 'void *' to a member function pointer of the appropriate type.
		template <class R>
		void toMember(R &to, const void *from);

		// Decide if we shall perform a member or nonmember call depending on what the function pointer looks like.
		inline bool memberCall(bool member, const void *fn);


		/**
		 * Generic implementation:
		 */


		/**
		 * Recurse through the parameter list until we reach the end.
		 */
		template <class Result, class Par, bool member>
		struct ParamHelp {
			template <class ... T>
			static void call(const void *fn, void **params, void *out, nat pos, T& ... args) {
				pos--;
				ParamHelp<Result, typename Par::PrevType, member>
					::call(fn, params, out, pos, *(typename Par::HereType *)params[pos], args...);
			}

			template <class ...T>
			static void callFirst(const void *fn, void **params, void *out, void *first, nat pos, T& ... args) {
				pos--;
				ParamHelp<Result, typename Par::PrevType, member>
					::callFirst(fn, params, out, first, pos, *(typename Par::HereType *)params[pos], args...);
			}
		};


		/**
		 * At the end. Perform a nonmember call with a result.
		 */
		template <class Result>
		struct ParamHelp<Result, Param<void, void>, false> {
			template <class ... T>
			static void call(const void *fn, void **, void *out, nat pos, T& ... args) {
				typedef Result CODECALL (*Fn)(T...);
				Fn p = (Fn)fn;
				new (storm::Place(out)) Result((*p)(args...));
			}

			template <class ...T>
			static void callFirst(const void *fn, void **params, void *out, void *first, nat pos, T& ... args) {
				call(fn, params, out, pos, first, args...);
			}
		};

		/**
		 * At the end. Perform a member call with a result.
		 */
		template <class Result>
		struct ParamHelp<Result, Param<void, void>, true> {
			class Dummy : NoCopy {};

			template <class U, class ... T>
			static IF_POINTER(U) call(const void *fn, void **, void *out, nat pos, U first, T& ... args) {
				typedef Result CODECALL (Dummy::*Fn)(T...);
				Fn p = null;
				toMember(p, fn);
				Dummy *f = (Dummy *)first;
				new (storm::Place(out)) Result((f->*p)(args...));
			}

			template <class U, class ... T>
			static IF_NOT_POINTER(U) call(const void *fn, void **, void *out, nat pos, U first, T& ... args) {
				typedef Result CODECALL (*Fn)(U, T...);
				Fn p = (Fn)fn;
				new (storm::Place(out)) Result((*p)(first, args...));
			}

			static void call(const void *fn, void **, void *out, nat pos) {
				typedef Result CODECALL (*Fn)();
				Fn p = (Fn)fn;
				new (storm::Place(out)) Result((*p)());
			}

			template <class ...T>
			static void callFirst(const void *fn, void **params, void *out, void *first, nat pos, T& ... args) {
				call(fn, params, out, pos, first, args...);
			}
		};


		/**
		 * At the end. Perform a nonmember call without a result.
		 */
		template <>
		struct ParamHelp<void, Param<void, void>, false> {
			template <class ... T>
			static void call(const void *fn, void **, void *out, nat pos, T& ... args) {
				typedef void CODECALL (*Fn)(T...);
				Fn p = (Fn)fn;
				(*p)(args...);
			}


			template <class ...T>
			static void callFirst(const void *fn, void **params, void *out, void *first, nat pos, T& ... args) {
				call(fn, params, out, pos, first, args...);
			}
		};


		/**
		 * At the end. Perform a member call without a result.
		 */
		template <>
		struct ParamHelp<void, Param<void, void>, true> {
			class Dummy : NoCopy {};

			template <class U, class ... T>
			static IF_POINTER(U) call(const void *fn, void **, void *out, nat pos, U first, T& ... args) {
				typedef void CODECALL (Dummy::*Fn)(T...);
				Fn p = null;
				toMember(p, fn);
				Dummy *f = (Dummy *)first;
				(f->*p)(args...);
			}

			template <class U, class ... T>
			static IF_NOT_POINTER(U) call(const void *fn, void **, void *out, nat pos, U first, T& ... args) {
				typedef void CODECALL (*Fn)(U, T...);
				Fn p = (Fn)fn;
				(*p)(first, args...);
			}

			static void call(const void *fn, void **, void *out, nat pos) {
				typedef void CODECALL (*Fn)();
				Fn p = (Fn)fn;
				(*p)();
			}

			template <class ...T>
			static void callFirst(const void *fn, void **params, void *out, void *first, nat pos, T& ... args) {
				call(fn, params, out, pos, first, args...);
			}
		};

		template <class Result, class Par>
		void call(const void *fn, bool member, void **params, void *first, void *result) {
			if (memberCall(member, fn)) {
				if (first) {
					ParamHelp<Result, Par, true>::callFirst(fn, params, result, first, Par::count);
				} else {
					ParamHelp<Result, Par, true>::call(fn, params, result, Par::count);
				}
			} else {
				if (first) {
					ParamHelp<Result, Par, false>::callFirst(fn, params, result, first, Par::count);
				} else {
					ParamHelp<Result, Par, false>::call(fn, params, result, Par::count);
				}
			}
		}

		/**
		 * Platform specific implementations:
		 */

#if defined(GCC) && defined(POSIX) && defined(X64)

		/**
		 * On GCC for Unix and X64, a member function pointer is actually 2 machine words. The first
		 * (64-bit) word is where the actual pointer is located, while the second word contains an
		 * offset to the vtable. Since we truncate function pointers to 1 machine word, the vtable
		 * pointer is unfortunatly lost. This is, however, fine in Storm since we know that the
		 * vtable is always located at offset 0. This is known since all objects with virtual
		 * function inherit from an object for which this is true, and we do not support multiple
		 * (virtual) inheritance.
		 *
		 * GCC abuses the function pointer a bit with regards to virtual dispatch as well. The
		 * function pointer is either a plain pointer to the machine code to execute (always aligned
		 * to at least 2 bytes) or an offset into the vtable of the object. The first case is easy:
		 * just call the function at the other end of the pointer. In case the function pointer is a
		 * vtable offset, we need to perform the vtable lookup through the object's vtable. We can
		 * distinguish between the two cases by examining the least significant bit of the
		 * pointer. If it is set (ie. if the address is odd), the pointer is actually an offset into
		 * the vtable +1. Otherwise it is a pointer.
		 *
		 * This was derived by examining the assembler output from GCC when compiling the file
		 * Experiments/membercall.cpp with 'g++ -S membercall.cpp'.
		 */

		template <class R>
		void toMember(R &to, const void *from) {
			// Set everything to zero first.
			memset(&to, 0, sizeof(R));
			// Then copy the pointer to the first word.
			memcpy(&to, &from, sizeof(void *));
		}

		inline bool memberCall(bool member, const void *fn) {
			// Do not attempt to perform a member call if the function pointer is not a vtable
			// reference. In some cases, especially when using optimizations, GCC makes full use of
			// its assumptions that 'this' pointers are not null and may thus crash 'too early' in
			// some cases.
			return member && (size_t(fn) & 0x1);
		}

#else
#error "Please provide a function pointer specifics for your platform!"
#endif

	}
}
