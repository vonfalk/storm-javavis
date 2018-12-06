#pragma once
#include <locale.h>

#ifdef WINDOWS
typedef locale_t _locale_t
#endif

namespace storm {
	STORM_PKG(core);

	/**
	 * Locale handling, so that we can get a sane locale if the system's locale is set to something
	 * other than the C locale.
	 */
	locale_t defaultLocale();

}
