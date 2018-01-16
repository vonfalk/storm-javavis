#include "stdafx.h"
#include "Seh.h"
#include "SafeSeh.h"
#include "Binary.h"
#include "StackFrame.h"
#include "Core/Str.h"
#include "Utils/Memory.h"
#include "Utils/StackInfo.h"

#if defined(WINDOWS) && defined(X86)

namespace code {
	namespace x86 {

		struct SEHFrame;

		// Find the Binary for a code segment we generated earlier.
		static Binary *codeBinary(const void *code) {
			size_t s = runtime::codeSize(code);
			if (s <= sizeof(void *))
				return null;

			const void *pos = (const char *)code + s - sizeof(void *);
			return *(Binary **)pos;
		}

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


		class SehInfo : public StackInfo {
		public:
			virtual void collect(::StackFrame &to, void *frame) const {
				// TODO: Detect frames that do not use SEH as well!
				to.data = null;

				util::Lock::L z(lock);
				for (std::set<Engine *>::const_iterator i = check.begin(), end = check.end(); i != end; ++i) {
					Engine *e = *i;
					void *code = runtime::findCode(*e, (void *)to.code);
					if (code)
						to.data = codeBinary(code);
				}
			}

			virtual bool format(std::wostream &to, const ::StackFrame &frame) const {
				if (frame.data) {
					Binary *b = (Binary *)frame.data;
					to << b->ownerName();
					return true;
				}
				return false;
			}

			void add(Engine &e) {
				util::Lock::L z(lock);

				// TODO: We should de-register Engine objects somehow...
				check.insert(&e);
			}

		private:
			// All Engine objects to check.
			std::set<Engine *> check;

			// Lock for updating the 'check' variable.
			mutable util::Lock lock;
		};

		void activateInfo(Engine &e) {
			static RegisterInfo<SehInfo> info;
			info->add(e);
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
		// TODO: Find any exception handlers here!
	}
	return ExceptionContinueSearch;
}

#endif
