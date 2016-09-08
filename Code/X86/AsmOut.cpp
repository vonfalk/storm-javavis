#include "stdafx.h"
#include "AsmOut.h"
#include "Asm.h"
#include "OpTable.h"

namespace code {
	namespace x86 {

#define OUTPUT(x) { op::x, &x ## Out }

		typedef void (*OutputFn)(Output *to, Instr *instr);

		void movOut(Output *to, Instr *instr) {
			ImmRegInstr8 op8 = {
				0xC6, 0,
				0x88,
				0x8A,
			};
			ImmRegInstr op = {
				0x0, 0xFF, // Not supported
				0xC7, 0,
				0x89,
				0x88,
			};
			immRegInstr(to, op8, op, instr->dest(), instr->src());
		}

		const OpEntry<OutputFn> outputMap[] = {
			OUTPUT(mov),
		};

		void output(Listing *src, Output *to) {
			static OpTable<OutputFn> t(outputMap, ARRAY_COUNT(outputMap));

			for (Nat i = 0; i < src->count(); i++) {
				Instr *instr = src->at(i);
				OutputFn fn = t[instr->op()];
				if (fn) {
					(*fn)(to, instr);
				} else {
					assert(false, L"Unsupported op-code!");
				}
			}
		}

	}
}
