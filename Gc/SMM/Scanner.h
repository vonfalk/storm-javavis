#pragma once

#if STORM_GC == STORM_GC_SMM

#include "AddrSet.h"
#include "Gc/Format.h"

namespace storm {
	namespace smm {

		/**
		 * Assorted scanners used in the SMM GC.
		 */


		/**
		 * Scan into an AddrSet, to keep track of ambiguous references.
		 */
		template <size_t size>
		struct ScanSummary {
			typedef int Result;
			typedef AddrSet<size> Source;

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

			// TODO: We want to generalize the fixHeader functions so that we don't have to add them everywhere.
			inline bool fixHeader1(GcType *header) {
				src.add(header);
				return false;
			}

			inline Result fix2(void **ptr) { return 0; }
			inline Result fixHeader2(GcType **header) { return 0; }
		};

	}
}

#endif
