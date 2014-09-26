#pragma once

#include <hash_map>

namespace stdext {
#ifndef NO_MFC
	//Hash value for hash_map
	inline size_t hash_value(const CString &s) {
		return hash_value((LPCTSTR)s);
	}
#endif

	inline size_t hash_value(const String &s) {
		return hash_value(s.c_str());
	}

}

using stdext::hash_map;