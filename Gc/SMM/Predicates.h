#pragma once

#if STORM_GC == STORM_GC_SMM

#include "Format.h"
#include "Scanner.h"

namespace storm {
	namespace smm {

		/**
		 * Some useful predicates while scanning.
		 */

		/**
		 * Only scan objects that contain weak references.
		 */
		template <class Scanner>
		struct OnlyWeak : public Scanner {
			OnlyWeak(typename Scanner::Source &source) : Scanner(source) {}

			inline ScanOption object(void *ptr, void *end) {
				if (fmt::objIsWeak(fmt::fromClient(ptr)))
					return Scanner::object(ptr, end);
				else
					return scanNone;
			}
		};


		/**
		 * Only scan non-weak references (i.e. only headers of weak arrays and normal objects).
		 */
		template <class Scanner>
		struct NoWeak : public Scanner {
			NoWeak(typename Scanner::Source &source) : Scanner(source) {}

			inline ScanOption object(void *ptr, void *end) {
				ScanOption opt = Scanner::object(ptr, end);
				if ((opt == scanAll) && fmt::objIsWeak(fmt::fromClient(ptr))) {
					opt = scanHeader;
				}
				return opt;
			}
		};


	}
}

#endif
