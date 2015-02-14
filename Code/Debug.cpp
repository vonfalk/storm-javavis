#include "stdafx.h"
#include "Debug.h"

// For debugging!
#include "Code/X86/Seh.h"

namespace code {

#if defined(X86) && defined(WINDOWS)

	// TODO: Implement this using the already existing backend!

	struct SehFrame {
		SehFrame *prev;
		void *fn;

		// From the storm code generation.
		void *block, *part;
	};

	void dumpStack() {
		SehFrame *top;
		__asm {
			mov eax, fs:[0];
			mov top, eax;
		};

		PLN("---- STACK DUMP ----");

		for (nat i = 0; i < 10; i++) {
			PLN("SEH at: " << top);
			nat addr = nat(top);
			if (addr == null || addr == 0xFFFFFFFF)
				break;

			PLN("Handler: " << top->fn << " (storm: " << &x86SafeSEH << ")");
			if (nat(top) % 4 != 0)
				PLN("ERROR: STACK IS NOT WORD-ALIGNED");

			if (top->fn == &x86SafeSEH) {
				void **ebp = (void **)(top + 1);
				PLN("Ebp: " << ebp);

				for (nat p = 0; p < 3; p++)
					PLN("Param " << p << ": " << ebp[2 + p]);
				for (nat v = 0; v < 5; v++)
					PLN("Local " << v << ": " << ebp[-5 - v]);
			}

			top = top->prev;
		}

		PLN("---- DONE ----");
	}

#endif

}
