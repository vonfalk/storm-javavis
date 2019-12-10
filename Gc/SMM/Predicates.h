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


		/**
		 * Template to force a scanner to always execute, regardless of any other predicates.
		 *
		 * Might not always work in case of nested scanners. Mainly intended for debugging.
		 */
		template <class Scanner>
		struct Always : public Scanner {
			Always(typename Scanner::Source &source) : Scanner(source) {}

			inline ScanOption object(void *ptr, void *end) {
				Scanner::object(ptr, end);
				return scanAll;
			}
		};


		/**
		 * Only scan the header.
		 */
		template <class Scanner>
		struct OnlyHeader : public Scanner {
			OnlyHeader(typename Scanner::Source &source) : Scanner(source) {}

			inline ScanOption object(void *ptr, void *end) {
				Scanner::object(ptr, end);
				return scanHeader;
			}
		};

	}
}

#endif
