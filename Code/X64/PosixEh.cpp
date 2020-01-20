#include "stdafx.h"
#include "PosixEh.h"
#include "Binary.h"
#include "Gc/DwarfTable.h"
#include "Utils/StackInfo.h"
#include "Core/Str.h"
#include "DwarfRegs.h"

#ifdef POSIX
#include <cxxabi.h>
#endif

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

		// Find the currently active part.
		static Nat stormFindPart(const FnData *data, size_t fn, size_t pc) {
			const FnPart *parts = getParts(data);

			Nat offset = pc - fn;
			Nat invalid = Part().key();

			// The entries are sorted by their 'offset' member. We can perform a binary search!
			// Note: if there is an entry where 'offset == parts[i].offset', we shall not select
			// that one since 'pc' points to the first instruction that was not executed.
			const FnPart *found = std::lower_bound(parts, parts + data->partCount, offset, PartCompare());
			if (found == parts) {
				// Before any part, nothing to do!
				return invalid;
			}
			found--;

			return found->part;
		}

		// Storage from phase 1 to phase 2. Binary compatible with parts of __cxa_exception in GCC.
		// Names from the GCC implementation for simplicity.

		/**
		 * GCC private exception struct. We need to be able to access various members in here during
		 * the exception handling. More specifically:
		 *
		 * - 'exceptionType' in 'isStormException' to be able to check the type of exception that was thrown.
		 * - 'adjustedPtr' in the personality function to store the resulting exception object in
		 *   phase 1. Otherwise __cxa_begin_catch() does not return the proper value in phase 2 of the
		 *   exception handling.
		 * - 'handlerSwitchValue' in the personality function to store the current part, so we don't
		 *   have to re-compute it in phase 2. This is not important for the implementation to function,
		 *   but GCC does something similar, so why not do the same and gain the small performance boost?
		 * - 'catcTemp' in the personality function to store the location we shall resume from so we
		 *   don't have to re-compute it in phase 2. Not important for the implementation to function,
		 *   but GCC does something similar. It is potentially dangerous to store a GC pointer in this
		 *   structure, as it is not scanned (it is allocated separately using malloc). However, during
		 *   the time the pointer is stored in here, a pointer into the same code block is pinned by
		 *   being on the execution stack anyway, so it is fine in this case.
		 */
		struct ExStore {
			std::type_info *exceptionType;
			void *exceptionDestructor;

			std::unexpected_handler unexpectedHandler;
			std::terminate_handler terminateHandler;

			void *nextException;
			int handlerCount;
			int handlerSwitchValue;
			const byte *actionRecord;
			const byte *languageSpecificData;
			void *catchTemp;
			void *adjustedPtr;

			struct _Unwind_Exception exception;
		};

		static RootObject *isStormException(_Unwind_Exception_Class type, struct _Unwind_Exception *data) {
			// Find the type info, and a pointer to the exception we're catching.
			const std::type_info *info = null;
			void *object = null;

			// For GCC:
			// This is 'GNUCC++\0' in hex.
			if (type == 0x474e5543432b2b00LL) {
				ExStore *store = BASE_PTR(ExStore, data, exception);
				info = store->exceptionType;
				object = store + 1;
			}

			// TODO: Handle LLVM exceptions as well. They have another type signature, but I believe
			// the layout of the 'data' is the same.

			// Unable to find type-info. The exception probably originated from somewhere we don't
			// know about.
			if (!info)
				return null;

			// This should actually be platform independent as long as the C++ Itanium ABI is used!
			// However, the Itanium ABI does not specify how to check if one typeid is a subclass
			// of another. That is GCC specific, but Clang most likely follows the same convention.

#if 0
			// This is likely slower and less appropriate than calling "do_catch" directly. It does,
			// however, seem to work, and shows what we're trying to accomplish.

			// We only support pointers.
			const abi::__pointer_type_info *ptr = dynamic_cast<const abi::__pointer_type_info *>(info);
			if (!ptr)
				return null;

			// Must point to a class.
			const abi::__class_type_info *srcClass = dynamic_cast<const abi::__class_type_info *>(ptr->__pointee);
			if (!srcClass)
				return null;

			object = *(void **)object;

#if 0
			// This is a bit clumsy and unreliable as we don't have the definition of the type __upcast_result.
			const abi::__class_type_info *target = (const abi::__class_type_info *)&typeid(storm::RootObject);

			// We don't have the type declaration for __class_type_info::__upcast_result.
			struct upcast_result {
				size_t data[10];
			} result;
			if (!srcClass->__do_upcast(target, object, (abi::__class_type_info::__upcast_result &)result))
				return null;
#else
			// This seems more appropriate. It calls __do_upcast eventually, but has access to the
			// upcast_result type, so it is likely more robust. Tell the implementation that the
			// pointer is const.
			if (!typeid(RootObject).__do_catch(srcClass, &object, abi::__pbase_type_info::__const_mask))
				return null;

			return (RootObject *)object;
#endif

#else
			// This is likely the best option. We avoid doing many dynamic_casts ourselves, and rely
			// on vtables instead. This is not ABI-compliant, as __do_catch is not mandated by the ABI.

			// Try to catch a storm::RootObject *.
			if (!typeid(RootObject * const).__do_catch(info, &object, abi::__pbase_type_info::__const_mask))
				return null;

			return *(RootObject **)object;
#endif

		}

		// The personality function called by the C++ runtime.
		_Unwind_Reason_Code stormPersonality(int version, _Unwind_Action actions, _Unwind_Exception_Class type,
											struct _Unwind_Exception *data, struct _Unwind_Context *context) {

			// We assume 'version == 1'.
			// 'type' is a 8 byte identifier. Equal to 0x47 4e 55 43 43 2b 2b 00 or 'GNUCC++\0' for exceptions from G++.
			// 'data' is compiler specific information about the exception. See 'unwind.h' for details.
			// 'context' is the unwinder state.

			// Note: using _Unwind_GetCFA seems to return the CFA of the previous frame.
			size_t fn = _Unwind_GetRegionStart(context);
			size_t pc = _Unwind_GetIP(context);
			size_t rbp = _Unwind_GetGR(context, DW_REG_RBP);

			const FnData *fnData = getData((const void *)fn);

			if (actions & _UA_SEARCH_PHASE) {
				// Phase 1: Search for handlers.

				// See if this function has any handlers at all.
				if (!fnData->owner->hasCatch())
					return _URC_CONTINUE_UNWIND;

				// See if this is an exception Storm can catch (somewhat expensive).
				RootObject *object = isStormException(type, data);
				if (!object)
					return _URC_CONTINUE_UNWIND;

				// Find if it is desirable to actually catch it.
				Nat part = stormFindPart(fnData, fn, pc);
				if (part == Part().key())
					return _URC_CONTINUE_UNWIND;

				Binary::Resume resume;
				if (!fnData->owner->hasCatch(part, object, resume))
					return _URC_CONTINUE_UNWIND;

				// Store things to phase 2, right above the _Unwind_Exception, just like GCC does:
				ExStore *store = BASE_PTR(ExStore, data, exception);

				// Adjusted pointer (otherwise __cxa_begin_catch won't work):
				store->adjustedPtr = object;

				// The following two are not too important, but we keep them at reasonable places as
				// to not confuse the GCC implementation:
				store->catchTemp = resume.ip;
				store->handlerSwitchValue = part;

				// Tell the system we have a handler! It will call us with _UA_HANDLER_FRAME later.
				return _URC_HANDLER_FOUND;
			} else if ((actions & _UA_CLEANUP_PHASE) && (actions & _UA_HANDLER_FRAME)) {
				// Phase 2: Cleanup, but resume here!

				// Read data from the exception object, like GCC does. We saved this data earlier.
				ExStore *store = BASE_PTR(ExStore, data, exception);
				Nat part = store->handlerSwitchValue;
				pc = (size_t)store->catchTemp;

				// Cleanup our frame.
				StackFrame frame(part, (void *)rbp);
				fnData->owner->cleanup(frame, part);

				// Get the exception. Since it is a pointer, we can deallocate directly.
				// Note that we store the dereferenced pointer inside the EH table, so we don't need
				// to dereference it again here!
				void *exception = __cxa_begin_catch(data);
				// We don't need the data to be alive anymore. We can deallocate it now!
				__cxa_end_catch();

				// Set up the context as desired. It should be unwound for us, so we just need to
				// modify RAX and IP, then we're good to go!
				// Note: It seems we can only set "RAX" and "RDX" here.
				_Unwind_SetGR(context, DW_REG_RAX, (_Unwind_Word)exception);
				_Unwind_SetIP(context, pc);

				return _URC_INSTALL_CONTEXT;
			} else if (actions & _UA_CLEANUP_PHASE) {
				// Phase 2: Cleanup!

				// Clean up what we can!
				Nat part = stormFindPart(fnData, fn, pc);
				if (part != Part().key()) {
					StackFrame frame(part, (void *)rbp);
					fnData->owner->cleanup(frame);
				}
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

