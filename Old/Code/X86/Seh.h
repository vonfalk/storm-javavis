#pragma once
#include "Utils/Platform.h"

#if defined(X86) && defined(SEH)
#include "SafeSEH.h"

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
