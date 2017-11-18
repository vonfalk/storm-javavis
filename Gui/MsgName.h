#pragma once

#ifdef GUI_WIN32

namespace gui {

	/**
	 * Utility for looking up the name of window messages.
	 */
	const wchar *msgName(UINT msg);

}

#endif
