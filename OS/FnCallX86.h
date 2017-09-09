#pragma once
#include "Utils/Platform.h"

namespace os {

	/**
	 * Implementation of the FnCall logic for X86. Included from 'FnCall.h'.
	 */

	namespace impl {

		// Params passed to all 'functions' inside the loop. All parameter lists must match exactly.
#define LOOP_PARAMS const void *fn, void **params, void **seq, void *result, nat index, void *first, void *src, void *dst

		// Number of elements appended to the beginning of 'params'.
#define SEQ_OFFSET 2

		// Perform one iteration of the call loop. Elements in 'seq' are assumed to jump back here when they are done.
		inline void NAKED doCallLoop(LOOP_PARAMS) {
			__asm {
				dec index;

				mov eax, seq;
				mov ecx, index;
				mov eax, [eax+ecx*4];

				jmp eax;
			}
		}

		// Initialize the call loop.
		inline void NAKED doCall(LOOP_PARAMS) {
			__asm {
				push ebp;
				mov ebp, esp;

				jmp doCallLoop;
			}
		}

		/**
		 * Perform the actual function call. One of these is placed last in the 'seq' array.
		 */

		// The general case.
		template <class Result>
		void NAKED execCall(LOOP_PARAMS) {
			__asm {
				// Result address is passed as the first parameter.
				push result;

				// Call the function.
				call fn;

				// Return from 'doCall'.
				mov esp, ebp;
				pop ebp;
				ret;
			}
		}

		// Special case for user-defined types as member functions.
		inline void NAKED execMemberUser(LOOP_PARAMS) {
			__asm {
				// In this case, the result is passed as parameter #2. Here, it is safe to assume
				// that the current first parameter is a pointer, since we're calling a member
				// function.
				pop eax;
				push result;
				push eax;

				// Call the function.
				call fn;

				// Return from 'doCall'.
				mov esp, ebp;
				pop ebp;
				ret;
			}
		}

		// Specialization for 'void'.
		template <>
		inline void NAKED execCall<void>(LOOP_PARAMS) {
			__asm {
				call fn;

				mov esp, ebp;
				pop ebp;
				ret;
			}
		}

		// Specialization for 'bool'.
		template <>
		inline void NAKED execCall<bool>(LOOP_PARAMS) {
			__asm {
				call fn;

				mov ecx, result;
				mov [ecx], al;

				mov esp, ebp;
				pop ebp;
				ret;
			}
		}

		// Specialization for 'int'.
		template <>
		inline void NAKED execCall<int>(LOOP_PARAMS) {
			__asm {
				call fn;

				mov ecx, result;
				mov [ecx], eax;

				mov esp, ebp;
				pop ebp;
				ret;
			}
		}

		// Specialization for 'nat'.
		template <>
		inline void NAKED execCall<nat>(LOOP_PARAMS) {
			__asm {
				call fn;

				mov ecx, result;
				mov [ecx], eax;

				mov esp, ebp;
				pop ebp;
				ret;
			}
		}

		// Specialization for 'long'.
		template <>
		inline void NAKED execCall<int64>(LOOP_PARAMS) {
			__asm {
				call fn;

				mov ecx, result;
				mov [ecx+0], eax;
				mov [ecx+4], edx;

				mov esp, ebp;
				pop ebp;
				ret;
			}
		}

		// Specialization for 'word'.
		template <>
		inline void NAKED execCall<nat64>(LOOP_PARAMS) {
			__asm {
				call fn;

				mov ecx, result;
				mov [ecx+0], eax;
				mov [ecx+4], edx;

				mov esp, ebp;
				pop ebp;
				ret;
			}
		}

		// Specialization for 'float'.
		template <>
		inline void NAKED execCall<float>(LOOP_PARAMS) {
			__asm {
				call fn;

				mov ecx, result;
				fstp DWORD PTR [ecx];

				mov esp, ebp;
				pop ebp;
				ret;
			}
		}

		// Specialization for 'double'.
		template <>
		inline void NAKED execCall<double>(LOOP_PARAMS) {
			__asm {
				call fn;

				mov ecx, result;
				fstp QWORD PTR [ecx];

				mov esp, ebp;
				pop ebp;
				ret;
			}
		}

		/**
		 * Push a single parameter on the stack, then jump back to 'doCallLoop'.
		 */

		// Helper for invoking a constructor (no exceptions in NAKED functions).
		template <class T>
		void create(void *dst, void *src) {
			new (storm::Place(dst)) T(*(T *)src);
		}

		// Generic parameter. Uses the constructor of the parameter to copy it.
		template <class Par>
		void NAKED pushParam(LOOP_PARAMS) {
			// We need the size from C++.
			src = (void *)((sizeof(Par) + 3) & ~size_t(0x3));

			__asm {
				// Allocate space on the stack and compute 'dst'.
				mov eax, src;
				sub esp, eax;
				mov dst, esp;

				// Read 'src' from 'params'.
				mov eax, params;
				mov ecx, index;
				sub ecx, SEQ_OFFSET;
				mov eax, [eax+ecx*4];
				mov src, eax;
			}

			// Call the constructor.
			create<Par>(dst, src);

			__asm {
				// Continue the parameter pushing loop.
				jmp doCallLoop;
			}
		}

		// Specialization for 'bool'.
		template <>
		inline void NAKED pushParam<bool>(LOOP_PARAMS) {
			__asm {
				mov eax, params;
				mov ecx, index;
				sub ecx, SEQ_OFFSET;
				mov eax, [eax+ecx*4];
				push [eax];
				jmp doCallLoop;
			}
		}

		// Specialization for 'char'.
		template <>
		inline void NAKED pushParam<char>(LOOP_PARAMS) {
			__asm {
				mov eax, params;
				mov ecx, index;
				sub ecx, SEQ_OFFSET;
				mov eax, [eax+ecx*4];
				push [eax];
				jmp doCallLoop;
			}
		}

		// Specialization for 'byte'.
		template <>
		inline void NAKED pushParam<byte>(LOOP_PARAMS) {
			__asm {
				mov eax, params;
				mov ecx, index;
				sub ecx, SEQ_OFFSET;
				mov eax, [eax+ecx*4];
				push [eax];
				jmp doCallLoop;
			}
		}

		// Specialization for 'int'.
		template <>
		inline void NAKED pushParam<int>(LOOP_PARAMS) {
			__asm {
				mov eax, params;
				mov ecx, index;
				sub ecx, SEQ_OFFSET;
				mov eax, [eax+ecx*4];
				push [eax];
				jmp doCallLoop;
			}
		}

		// Specialization for 'nat'.
		template <>
		inline void NAKED pushParam<nat>(LOOP_PARAMS) {
			__asm {
				mov eax, params;
				mov ecx, index;
				sub ecx, SEQ_OFFSET;
				mov eax, [eax+ecx*4];
				push [eax];
				jmp doCallLoop;
			}
		}

		// Specialization for 'long'.
		template <>
		inline void NAKED pushParam<int64>(LOOP_PARAMS) {
			__asm {
				mov eax, params;
				mov ecx, index;
				sub ecx, SEQ_OFFSET;
				mov eax, [eax+ecx*4];
				push [eax+4];
				push [eax+0];
				jmp doCallLoop;
			}
		}

		// Specialization for 'word'.
		template <>
		inline void NAKED pushParam<nat64>(LOOP_PARAMS) {
			__asm {
				mov eax, params;
				mov ecx, index;
				sub ecx, SEQ_OFFSET;
				mov eax, [eax+ecx*4];
				push [eax+4];
				push [eax+0];
				jmp doCallLoop;
			}
		}

		// Specialization for 'float'.
		template <>
		inline void NAKED pushParam<float>(LOOP_PARAMS) {
			__asm {
				mov eax, params;
				mov ecx, index;
				sub ecx, SEQ_OFFSET;
				mov eax, [eax+ecx*4];
				push [eax];
				jmp doCallLoop;
			}
		}

		// Specialization for 'double'.
		template <>
		inline void NAKED pushParam<double>(LOOP_PARAMS) {
			__asm {
				mov eax, params;
				mov ecx, index;
				sub ecx, SEQ_OFFSET;
				mov eax, [eax+ecx*4];
				push [eax+4];
				push [eax+0];
				jmp doCallLoop;
			}
		}

		inline void NAKED pushFirst(LOOP_PARAMS) {
			__asm {
				push first;
				jmp doCallLoop;
			}
		}

#undef READ_SRC

		template <class T>
		inline bool isUser() {
			return KindOf<T>::v == TypeKind::userTrivial
				|| KindOf<T>::v == TypeKind::userComplex;
		}

		/**
		 * Handle pointers/references properly.
		 */
		template <class T>
		struct CallInfo {
			static void *param() {
				return &pushParam<T>;
			}

			static void *exec(bool member) {
				if (isUser<T>() && member)
					return &execMemberUser;
				else
					return &execCall<T>;
			}
		};

		template <class T>
		struct CallInfo<T *> {
			static void *param() {
				return &pushParam<nat>;
			}

			static void *exec(bool member) {
				return &execCall<nat>;
			}
		};

		template <class T>
		struct CallInfo<T &> {
			static void *param() {
				return &pushParam<nat>;
			}

			static void *exec(bool member) {
				return &execCall<nat>;
			}
		};


		/**
		 * Generate a sequence of 'functions' which properly construct a stack frame for the call.
		 */

		template <class Par>
		void fillSeq(void **seq) {
			seq[Par::count - 1] = CallInfo<Par::HereType>::param();
			fillSeq<Par::PrevType>(seq);
		}

		template <>
		inline void fillSeq<Param<void, void>>(void **) {}

		template <class Result, class Par>
		void call(const void *fn, bool member, void **params, void *first, void *result) {
			void *seq[Par::count + SEQ_OFFSET];
			fillSeq<Par>(seq + SEQ_OFFSET);

			nat pos = SEQ_OFFSET;
			if (first)
				seq[--pos] = &pushFirst;
			seq[--pos] = CallInfo<Result>::exec(member);

			doCall(fn, params, seq, result, Par::count + SEQ_OFFSET, first, null, null);
		}

#undef SEQ_OFFSET
#undef LOOP_PARAMS

	}
}
