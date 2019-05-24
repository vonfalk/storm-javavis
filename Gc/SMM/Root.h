#pragma once

#if STORM_GC == STORM_GC_SMM

#include "InlineSet.h"

namespace storm {
	namespace smm {

		/**
		 * A root that can be registered in the GC.
		 *
		 * SMM supports two types of roots, exact roots and ambiguous roots. Exact roots are assumed
		 * to contain pointers to the start of objects, while ambiguous roots may point anywhere
		 * inside objects, or be other types than actual pointers. Objects referred to by ambiguous
		 * roots are pinned, and will not move. Such roots are assumed to be rather short-lived.
		 *
		 * Roots always refer to some chunk of memory, which is assumed to contain an array of pointers.
		 */
		class Root : public SetMember<Root> {
		public:
			// Create a root.
			Root(void **data, size_t count)
				: data(data), count(count) {}

			template <class Scanner>
			typename Scanner::Result scan(typename Scanner::Source &source);

		private:
			// Data array.
			void **data;
			size_t count;
		};


		template <class Scanner>
		typename Scanner::Result Root::scan(typename Scanner::Source &source) {
			Scanner s(source);
			typename Scanner::Result r = typename Scanner::Result();
			for (size_t i = 0; i < count; i++) {
				if (s.fix1(data[i])) {
					r = s.fix2(data + i);
					if (r != typename Scanner::Result())
						return r;
				}
			}
			return r;
		}

	}
}

#endif
