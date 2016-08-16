#include "stdafx.h"
#include "Core/Gen/CppTypes.h"
#include "Core/Runtime.h"

/**
 * Template for the generated cpp file.
 */

// INCLUDES

// TYPE_GLOBALS

// TEMPLATE_GLOBALS

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
			{ null, null, CppType::superNone, 0, CppSize::invalid, null, typeNone, null },
		};
		return types;
	}

	static CppTemplate *cppTemplates() {
		static CppTemplate templates[] = {
			// CPP_TEMPLATES
			{ null, null, null },
		};
		return templates;
	}

	static CppThread *cppThreads() {
		static CppThread threads[] = {
			// CPP_THREADS
			{ null, null, null },
		};
		return threads;
	}

	const CppWorld *cppWorld() {
		static CppWorld w = {
			cppTypes(),
			cppTemplates(),
			cppThreads(),
		};
		return &w;
	}

}

#pragma optimize ("", off)
