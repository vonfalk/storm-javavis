#include "stdafx.h"
#include "Seh.h"
#include "Binary.h"
#include "StackFrame.h"
#include "Utils/Memory.h"

#if defined(WINDOWS) && defined(X86)

namespace code {
	namespace x86 {

		struct SEHFrame;

		// Stack frame we're implementing.
		class Frame : public StackFrame {
		public:
			Frame(SEHFrame *frame);

			virtual void *toPtr(size_t offset);

		private:
			SEHFrame *frame;
		};

		// The SEH frame on the stack.
		struct SEHFrame {
			// SEH chain.
			SEHFrame *prev;
			const void *sehHandler;

			// The Binary object that owns this code.
			// This is a Binary ** to allow the Gc to move the Binary object even while
			// the function has a frame on the stack.
			Binary **owner;

			// The topmost active part.
			size_t activePart;

			// Current EBP points to this.
			void *lastEbp;

			// Get EBP.
			inline void *ebp() {
				return &lastEbp;
			}

			// Create from EBP.
			static SEHFrame *fromEbp(void *ebp) {
				byte *p = (byte *)ebp;
				return (SEHFrame *)(p - OFFSET_OF(SEHFrame, lastEbp));
			}

			// Create from SEH frame.
			static SEHFrame *fromSEH(void *seh) {
				byte *p = (byte *)seh;
				return (SEHFrame *)(p - OFFSET_OF(SEHFrame, prev));
			}

			// Cleanup this frame.
			void cleanup() {
				if (owner && *owner) {
					Frame f(this);
					(*owner)->cleanup(f);
				} else {
					WARNING(L"Using SEH, but no link to the metadata provided!");
				}
			}

		};


		Frame::Frame(SEHFrame *f) : StackFrame(f->activePart), frame(f) {}

		void *Frame::toPtr(size_t offset) {
			int o = offset;
			byte *ebp = (byte *)frame->ebp();
			return ebp + o;
		}

	}
}

// No proper definition found...
#define EXCEPTION_UNWINDING 2

using namespace code::x86;

extern "C"
EXCEPTION_DISPOSITION __cdecl x86SEH(_EXCEPTION_RECORD *er, void *frame, _CONTEXT *ctx, void *dispatchCtx) {
	if (er->ExceptionFlags & EXCEPTION_UNWINDING) {
		SEHFrame *f = SEHFrame::fromSEH(frame);
		f->cleanup();
	} else {
		// Find any exception handlers here!
		TODO(L"Find handlers.");
	}
	return ExceptionContinueSearch;
}

#endif
