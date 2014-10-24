#pragma once

#include <hash_map>
#include <hash_set>

namespace stdext {

	inline size_t hash_value(const String &s) {
		return hash_value(s.c_str());
	}

}

using stdext::hash_map;
using stdext::hash_set;

