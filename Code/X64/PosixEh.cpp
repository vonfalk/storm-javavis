#include "stdafx.h"
#include "PosixEh.h"
#include "Binary.h"
#include "Gc/DwarfTable.h"
#include "Utils/StackInfo.h"
#include "Core/Str.h"

namespace code {
	namespace x64 {

#ifdef POSIX

#ifndef GCC
#error "This is currently untested on other compilers than GCC"
#endif

#ifdef __USING_SJLJ_EXCEPTIONS__
#error "Storm does not support SJLJ exceptions. Please use DWARF exceptions instead!"
#endif

		// Struct used as a parameter to '_Unwind_Find_FDE'.
		struct dwarf_eh_bases {
			void *tbase;
			void *dbase;
			void *func;
		};

		// Previous version of the _Unwind_Find_FDE function (present in libgcc.so). Put inside a
		// class to use the one-time initialization without additional locks.
		struct PrevFindFDE {
			typedef FDE *(*Ptr)(void *pc, struct dwarf_eh_bases *bases);
			Ptr ptr;

			PrevFindFDE() {
				ptr = (Ptr)dlsym(RTLD_NEXT, "_Unwind_Find_FDE");
				if (!ptr) {
					printf("Failed to initialize exception handling (no _Unwind_Find_FDE found)\n");
					printf("Did you compile Storm using GCC with DWARF2 unwind information?\n");
					exit(250);
				}
			}
		};

		PrevFindFDE prevFDE;

		// Hook into the exception resolution system so that we can provide our generated DWARF
		// frames when required.
		extern "C" SHARED_EXPORT FDE *_Unwind_Find_FDE(void *pc, struct dwarf_eh_bases *bases) {
			// Try asking the standard library first.
			FDE *found = (*prevFDE.ptr)(pc, bases);
			if (found)
				return found;

			// If the standard library did not know of the current 'pc', then we try to find it
			// inside the code generated by Storm!
			found = dwarfTable().find(pc);

			if (found && bases) {
				bases->tbase = null;
				bases->dbase = null;
				bases->func = (void *)found->codeStart();
			}

			return found;
		}

		/**
		 * Description of the data at the end of each function.
		 */
		struct FnData {
			// Number of entries in the part table.
			size_t partCount;

			// The binary that contains the rest of the unwinding information for this Binary object.
			Binary *owner;

			// Pointer to an array of updater objects for this function.
			Array<Reference *> *refs;
		};

		/**
		 * Description of a single entry in the part table.
		 */
		struct FnPart {
			// At which offset do we start?
			Nat offset;

			// Which part?
			Nat part;
		};

		// Get the FnData from a function.
		static const FnData *getData(const void *fn) {
			size_t size = runtime::codeSize(fn);
			const byte *end = (const byte *)fn + size;
			return (const FnData *)(end - sizeof(FnData));
		}

		// Get the part data from a function.
		static const FnPart *getParts(const FnData *data) {
			const FnPart *end = (const FnPart *)data;
			return end - data->partCount;
		}

		// Compare object.
		struct PartCompare {
			bool operator()(const FnPart &a, Nat offset) const {
				return a.offset < offset;
			}
		};

		// Our representation of a stack frame.
		class StackFrame : public code::StackFrame {
		public:
			StackFrame(Nat part, void *rbp) : code::StackFrame(part), rbp((byte *)rbp) {}

			virtual void *toPtr(size_t offset) {
				ssize_t delta = offset;
				return rbp + delta;
			}

		private:
			byte *rbp;
		};

		// Perform cleanup using the tables present in the code.
		static void stormCleanup(size_t fn, size_t pc, size_t rbp) {
			const FnData *data = getData((const void *)fn);
			const FnPart *parts = getParts(data);

			Nat offset = pc - fn;
			Nat invalid = Part().key();

			// The entries are sorted by their 'offset' member. We can perform a binary search!
			// Note: if there is an entry where 'offset == parts[i].offset', we shall not select
			// that one since 'pc' points to the first instruction that was not executed.
			const FnPart *found = std::lower_bound(parts, parts + data->partCount, offset, PartCompare());
			if (found == parts) {
				// Before any part, nothing to do!
				return;
			}
			found--;
			if (found->part == invalid) {
				// A part that specifies outside the prolog and epilog. Nothing to do!
				return;
			}

			// Create a stack frame and pass it on to the Binary object for cleanup.
			StackFrame frame(found->part, (void *)rbp);
			data->owner->cleanup(frame);
		}

		static bool isStormException(_Unwind_Exception_Class type, struct _Unwind_Exception *data) {
			// TODO: We probably want to handle LLVM exceptions as well!
			// This is 'GNUCC++\0' in hex.
			if (type != 0x474e5543432b2b00LL)
				return false;

			byte *dataPtr = (byte *)data;
			std::type_info *info = *(std::type_info **)(dataPtr + 10*sizeof(void *));
			PVAR(info->name());

			void *obj = dataPtr + sizeof(_Unwind_Exception);
			// What is "outer"? I'll pass 0.
			// typeid(storm::RootObject*).__do_catch(info, &obj, 0);

			return true;
		}

		// The personality function called by the C++ runtime.
		_Unwind_Reason_Code stormPersonality(int version, _Unwind_Action actions, _Unwind_Exception_Class type,
											struct _Unwind_Exception *data, struct _Unwind_Context *context) {

			// We assume 'version == 1'.
			// 'type' is a 8 byte identifier. Equal to 0x47 4e 55 43 43 2b 2b 00 or 'GNUCC++\0' for exceptions from G++.
			// 'data' is compiler specific information about the exception. See 'unwind.h' for details.
			// 'context' is the unwinder state.

			if (actions & _UA_SEARCH_PHASE) {
				// Phase 1: Search for handlers.
				if (isStormException(type, data)) {
					PLN(L"Storm exception!");
				}
				// TODO: Return _URC_HANDLER_FOUND if we know how to handle it!
				return _URC_CONTINUE_UNWIND;
			} else if (actions & _UA_CLEANUP_PHASE) {
				// Phase 2: Cleanup!
				// if (actions & _UA_HANDLER_FRAME), we should return _URC_INSTALL_CONTEXT.

				// Clean up what we can! (register #6 is RBP)
				// Note: using _Unwind_GetCFA seems to return the CFA of the previous frame.
				stormCleanup(_Unwind_GetRegionStart(context), _Unwind_GetIP(context), _Unwind_GetGR(context, 6));
				return _URC_CONTINUE_UNWIND;
			} else {
				// Just pretend we did something useful...
				printf("WARNING: Personality function called with an unknown action!\n");
				return _URC_CONTINUE_UNWIND;
			}
		}

#else

		void stormPersonality() {}

#endif

		/**
		 * Function lookup using the Dwarf table.
		 */

		class DwarfInfo : public StackInfo {
		public:
			virtual bool format(std::wostream &to, const ::StackFrame &frame) const {
				FDE *fde = dwarfTable().find(frame.code);
				if (!fde)
					return false;

				char *start = (char *)fde->codeStart();
				if (fde->codeSize() != runtime::codeSize(start)) {
					// Something is fishy!
					WARNING(L"This does not seem like code we know...");
					return false;
				}
				start += runtime::codeSize(start) - 2*sizeof(void *);
				Binary *owner = *(Binary **)start;
				Str *name = owner->ownerName();
				if (name) {
					to << name->c_str();
				} else {
					to << L"<unnamed Storm function>";
				}
				return true;
			}
		};

		void activateInfo() {
			static RegisterInfo<DwarfInfo> info;
		}

	}
}

