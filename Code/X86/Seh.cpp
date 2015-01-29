#include "stdafx.h"
#include "Seh.h"

#if defined(WIN32) && defined(SEH)

#include "Binary.h"
#include "TransformX86.h"

using namespace code::machineX86;

namespace code {

	namespace machineX86 {

		NT_TIB *getTIB() {
			NT_TIB *tib;
			__asm {
				// read the "self" member from "fs:0x18"
				mov eax, fs:0x18;
				mov tib, eax;
			}
			assert(tib == tib->Self);
			return tib;
		}

		// The SEH frame on the stack.
		struct SEHFrame {

			// SEH chain, only valid if using SEH
			SEHFrame *prev;
			void *sehFn;

			// The binary object that owns this code.
			code::Binary *owner;

			// The topmost active scope.
			nat activeBlock;

			// Current EBP points to this!
			void *lastEbp;

			// Get EBP.
			inline void *ebp() {
				return &lastEbp;
			}
			inline const void *ebp() const {
				return &lastEbp;
			}

			// Create from ebp.
			static SEHFrame *fromEbp(void *ebp) {
				byte *p = (byte *)ebp;
				return (SEHFrame *)(p - OFFSET_OF(SEHFrame, lastEbp));
			}

			// Create from SEH frame.
			static SEHFrame *fromSEH(void *seh) {
				byte *p = (byte *)seh;
				return (SEHFrame *)(p - OFFSET_OF(SEHFrame, prev));
			}
		};

		void cleanupFrame(SEHFrame *frame) {
			frame->owner->destroyFrame(frame);
		}

	}

	namespace machine {

		nat activeBlock(const StackFrame &frame, const FnMeta *data) {
			return frame->activeBlock;
		}

		// Get the parent stack frame.
		StackFrame parentFrame(const StackFrame &frame, const FnMeta *data) {
			return SEHFrame::fromEbp(frame->lastEbp);
		}

		VarInfo variableInfo(const StackFrame &frame, const FnMeta *data, nat variableId) {
			assert(variableId < data->infoCount);
			const Meta::Var &v = data->info[variableId];
			byte *ebp = (byte *)frame->ebp();

			VarInfo info = { ebp + v.offset, v.freeFn, FreeOpt(v.freeOpt) };
			return info;
		}
	}
}

// No proper definition found...
#define EXCEPTION_UNWINDING 2

extern "C"
EXCEPTION_DISPOSITION __cdecl x86SEH(_EXCEPTION_RECORD *exception, void *frame, _CONTEXT *context, void *dispatchContext) {
	if (exception->ExceptionFlags & EXCEPTION_UNWINDING) {
		SEHFrame *current = SEHFrame::fromSEH(frame);

		// Unwind stack frame.
		cleanupFrame(current);
		return ExceptionContinueSearch;
	} else {
		// Find any exception handlers (not implemented yet...)
		TODO("Find handlers");
		return ExceptionContinueSearch;
	}
}

#endif
