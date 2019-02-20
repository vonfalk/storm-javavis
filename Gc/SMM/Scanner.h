#pragma once

#if STORM_GC == STORM_GC_SMM

#include "AddrSet.h"
#include "Gc/Format.h"

namespace storm {
	namespace smm {

		/**
		 * Assorted scanners used in the SMM GC.
		 */

		// Macro used to add 'fixHeader1' and 'fixHeader2' to call 'fix1' and 'fix2' respectively.
#define SCAN_FIX_HEADER													\
		inline bool fixHeader1(GcType *header) { return fix1(header); } \
		inline Result fixHeader2(GcType **header) { return fix2((void **)header); }


		/**
		 * Scan into an AddrSet, to keep track of ambiguous references.
		 */
		template <class AddrSet>
		struct ScanSummary {
			typedef int Result;
			typedef AddrSet Source;

			Source &src;

			ScanSummary(Source &source) : src(source) {}

			inline bool skip(fmt::Obj *ptr) {
				// For testing.
				return false;
			}

			inline bool fix1(void *ptr) {
				src.add(ptr);
				return false;
			}

			inline Result fix2(void **ptr) { return 0; }

			SCAN_FIX_HEADER
		};

	}
}

#endif
