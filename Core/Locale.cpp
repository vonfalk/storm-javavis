#include "stdafx.h"
#include "Locale.h"

namespace storm {

#ifdef WINDOWS
	static locale_t newlocale(int type, const char *locale, locale_t) {
		return _create_locale(type, locale);
	}
#endif

	// The default C locale. Created on demand. Access using the function 'cLocale' instead of through here.
	static locale_t locale;
	static nat hasLocale = false;
	static util::Lock localeLock;

	// Get the default C locale, so that we can format floats consistently regardless of the current locale...
	locale_t defaultLocale() {
		if (atomicRead(hasLocale) == 0) {
			util::Lock::L z(localeLock);
			if (atomicRead(hasLocale) == 0) {
				locale = newlocale(LC_ALL, "C", NULL);
				atomicWrite(hasLocale, 1);
			}
		}

		return locale;
	}

}
