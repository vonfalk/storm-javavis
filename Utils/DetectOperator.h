#pragma once
#include "Templates.h"

namespace detect_operator {
	// Note: sizeof(NoType) == sizeof(bool)*2
	template <class T>
	NoType &operator ==(const T &, const T &);

	template <class U>
	static const U &create();

	template <class T>
	struct equals {
		enum { value = sizeof(create<T>() == create<T>()) != sizeof(NoType) };
	};

	template <>
	struct equals<void> {
		enum { value = false };
	};
}
