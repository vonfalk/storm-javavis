#pragma once

#if STORM_GC == STORM_GC_SMM

#include "Format.h"

namespace storm {
	namespace smm {

		/**
		 * Some useful predicates while scanning.
		 */

		/**
		 * Check if an object is a weak array.
		 */
		struct IfWeak {
			inline fmt::ScanOption operator ()(void *ptr, void *) const {
				return fmt::objIsWeak(fmt::fromClient(ptr)) ? fmt::scanAll : fmt::scanNone;
			}
		};


		/**
		 * Check if an object is not a weak array.
		 */
		struct IfNotWeak {
			inline fmt::ScanOption operator ()(void *ptr, void *) const {
				return fmt::objIsWeak(fmt::fromClient(ptr)) ? fmt::scanHeader : fmt::scanAll;
			}
		};

		/**
		 * Create the logical AND of two predicates.
		 */
		template <class A, class B>
		struct IfBoth {
			A a;
			B b;

			IfBoth(const A &a, const B &b) : a(a), b(b) {}

			inline fmt::ScanOption operator ()(void *ptr, void *end) const {
				return fmt::ScanOption(::min(int(a(ptr, end)), int(b(ptr, end))));
			}
		};


	}
}

#endif
