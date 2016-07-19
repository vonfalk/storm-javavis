#include "stdafx.h"
#include "CppTypes.h"
#include "Engine.h"

/**
 * Template for the generated cpp file.
 */

// INCLUDES

// TYPE_GLOBALS

// THREAD_GLOBALS

// Turn off optimizations in this file. It takes quite a long time, and since it is only executed
// once during compiler startup, it is not very useful to optimize these functions. Especially not
// during testing!
#pragma optimize("", off)

namespace storm {

	// Invalid size.
	const CppSize CppSize::invalid = { -1, -1, -1, -1 };

	// Invalid offset.
	const CppOffset CppOffset::invalid = { -1, -1 };

	// Wrap a destructor call.
	template <class T>
	void destroy(T *obj) {
		obj->~T();
	}

	static CppType *cppTypes() {
		// PTR_OFFSETS

		static CppType types[] = {
			// CPP_TYPES
			{ null, 0, CppSize::invalid, null, null },
		};
		return types;
	}

	static CppThread *cppThreads() {
		static CppThread threads[] = {
			// CPP_THREADS
			{ null },
		};
		return threads;
	}

	const CppWorld *cppWorld() {
		static CppWorld w = {
			cppTypes(),
			cppThreads(),
		};
		return &w;
	}

}

#pragma optimize ("", off)
