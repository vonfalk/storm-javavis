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

			inline bool fix1(void *ptr) {
				src.add(ptr);
				return false;
			}

			inline Result fix2(void **ptr) { return 0; }

			SCAN_FIX_HEADER
		};


		/**
		 * Scan into a number of AddrSets to keep track of ambiguous references.
		 */
		template <class AddrSet>
		struct ScanSummaries {
			typedef int Result;
			typedef vector<AddrSet> Source;

			Source &src;

			ScanSummaries(Source &source) : src(source) {}

			inline bool fix1(void *ptr) {
				for (Source::iterator i = src.begin(), end = src.end(); i != end; ++i)
					i->add(ptr);
				return false;
			}

			inline Result fix2(void **ptr) { return 0; }

			SCAN_FIX_HEADER
		};


		/**
		 * Scan objects, updating any references that currently point to forwarding references.
		 */
		template <class Predicate>
		struct UpdateFwd {
			typedef int Result;
			typedef Predicate Source;

			Predicate &predicate;

			UpdateFwd(Predicate &predicate) : predicate(predicate) {}

			inline bool fix1(void *ptr) {
				return predicate(ptr);
			}

			inline Result fix2(void **ptr) const {
				fmt::Obj *o = fmt::fromClient(*ptr);
				// This will not update 'ptr' if it isn't a forwarding object.
				objIsFwd(o, ptr);
				return 0;
			}

			SCAN_FIX_HEADER
		};


		/**
		 * Perform some other scanning, and produce an address summary of all contained pointers.
		 */
		template <class AddrSet, class Other>
		struct WithSummary {
			typedef typename Other::Result Result;
			struct Source {
				AddrSet &set;
				typename Other::Source &src;

				Source(AddrSet &set, typename Other::Source &src) : set(set), src(src) {}
			};

			AddrSet &set;
			Other other;

			WithSummary(Source source) : set(source.set), other(source.src) {}

			inline bool fix1(void *ptr) {
				set.add(ptr);
				return other.fix1(ptr);
			}

			inline Result fix2(void **ptr) {
				return other.fix2(ptr);
			}

			inline bool fixHeader1(GcType *header) {
				set.add(header);
				return other.fixHeader1(header);
			}

			inline Result fixHeader2(GcType **header) {
				return other.fixHeader2(header);
			}
		};

	}
}

#endif
