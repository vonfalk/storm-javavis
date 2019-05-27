#pragma once

#if STORM_GC == STORM_GC_SMM

#include "Format.h"

namespace storm {
	namespace smm {

		/**
		 * Some useful predicates while scanning.
		 */


		/**
		 * Check if an object is not a weak array.
		 */
		struct IfNotWeak {
			inline bool operator ()(void *ptr, void *) const {
				return fmt::objHeader(fmt::fromClient(ptr))->type != GcType::tWeakArray;
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

			inline bool operator ()(void *ptr, void *end) const {
				return a(ptr, end)
					&& b(ptr, end);
			}
		};


	}
}

#endif
