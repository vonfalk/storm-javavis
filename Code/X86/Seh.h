#pragma once

#if defined(X86) && defined(SEH)

extern "C" void __stdcall x86SafeSEH();

namespace code {
	namespace machineX86 {

		// Get the thread information block.
		NT_TIB *getTIB();

		// Frame on the stack.
		struct SEHFrame;
	}

	namespace machine {
		typedef machineX86::SEHFrame *StackFrame;
	}
}

#endif