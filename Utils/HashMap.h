#pragma once

// Note: This is approximate...
#if defined(VISUAL_STUDIO) && VISUAL_STUDIO < 2013

#include <hash_map>
#include <hash_set>

namespace stdext {

	inline size_t hash_value(const String &s) {
		return hash_value(s.c_str());
	}

}

using stdext::hash_map;
using stdext::hash_set;

#else

#include <unordered_map>
#include <unordered_set>

template <class K, class V>
using hash_map = std::unordered_map<K, V>;
template <class K>
using hash_set = std::unordered_set<K>;

#endif
