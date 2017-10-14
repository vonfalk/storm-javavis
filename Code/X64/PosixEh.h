#pragma once
#include "Utils/Platform.h"
#include "DwarfEh.h"
#ifdef POSIX
#include <unwind.h>

namespace code {
	namespace x64 {

		/**
		 * Implementation of the runtime required to properly use the exceptions generated in
		 * DwarfEh.h. Currently specific to POSIX-like systems.
		 */

		// Personality function for Storm.
		_Unwind_Reason_Code stormPersonality(int version, _Unwind_Action actions, _Unwind_Exception_Class type,
											_Unwind_Exception *data, _Unwind_Context *context);

#ifdef GCC

		/**
		 * GCC specific things:
		 *
		 * Idea: Due to the way GCC stores these objects, we should make sure that all Chunks we
		 * store contain a FDE with an address of zero (or at least close to zero), so that our
		 * chunks are last in the internal list and therefore searched last. This is vital, since
		 * our chunks may span a quite large range of addresses that possibly overlap with other
		 * code in the system. By being last in the list, we do not accidentally override something
		 * important, and we can keep our 'pc_begin' constant so that we do not break the internal
		 * representation when we move objects around.
		 */

		/**
		 * Struct holding sorted records.
		 */
		struct SortedRecords {
			Record *original; // We know this is the case for our usage.
			size_t count;
			Record **sorted;
		};

		/**
		 * Struct compatible with 'object' from 'libgcc'. We can ignore much of the struct, but
		 * we need to be able to read and write the unions 'u' and 's'.
		 */
		struct GCCObject {
			void *pc_begin;
			void *tbase;
			void *dbase;

			union {
				Record *single;
				Record **array;
				SortedRecords *sorted;
			} u;

			union {
				struct {
					unsigned long sorted : 1;
					unsigned long from_array : 1;
					unsigned long mixed_encoding : 1;
					unsigned long encoding : 8;
					unsigned long count : 21;
				} b;
				size_t all;
			} s;

			// There are a maximum of two more pointers here, but depending on compilation flags
			// for 'libgcc' their layout is not known here. Therefore, we can not use them (and
			// we do not need to either).
			void *fde_end;
			void *next;
		};

#endif

	}
}
#endif
