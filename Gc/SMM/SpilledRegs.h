#pragma once

#if STORM_GC == STORM_GC_SMM

namespace storm {
	namespace smm {

#if defined(X86)
		// Need to store: ebx, ebp, esi, edi
		static const size_t spillCount = 4;
#elif defined(X64)
		// On Windows, rcx, rdx, r8 and r9 are used for parameters
		// Need to store: rbx, rbp, rdi, rsi, r12, r13, r14, r15

		// On Linux, rdi, rsi, rdx, rcx, r8 and r9 are used for parameters
		// Need to store: rbx, rbp, r10, r11, r12, r13, r14, r15 (r10 can be skipped)
		static const size_t spillCount = 8;
#else
#error "I don't know what system you are running on!"
#endif

		/**
		 * Registers spilled to the stack by us, so that we can properly scan any references that
		 * are stored in callee-saved registers on entry to the GC.
		 *
		 * We could use setjmp for this, but sadly setjmp doesn't necessarily store all registers in
		 * the jmp_buf. More precisely, on 64-bit Unix systems, the base pointer seems to be
		 * obfuscated with an xor operation before being stored in the jmp_buf, causing us to fail
		 * to scan any references stored in that register on entry to the GC. We solve this by
		 * implementing our own register spilling, as there are no guarantees regarding what the
		 * jmp_buf contains, and in libunwind's implementation it contains almost nothing.
		 */
		struct SpilledRegs {
			size_t reg[spillCount];
		};

		// Initialize spilled registers.
		extern "C" void spillRegs(SpilledRegs *to);

	}
}

#endif
