#include "stdafx.h"
#include "Core/Gen/CppTypes.h"
#include "Core/Runtime.h"

/**
 * Template for the generated cpp file.
 */

// INCLUDES

// PRIMITIVE_GLOBALS

// TYPE_GLOBALS

// TEMPLATE_GLOBALS

// THREAD_GLOBALS

// Turn off optimizations in this file. It takes quite a long time, and since it is only executed
// once during compiler startup, it is not very useful to optimize these functions. Especially not
// during testing!
#pragma optimize("", off)

namespace storm {

	struct CppMeta {
		static CppType *cppTypes();
		static CppFunction *cppFunctions();
		static CppTemplate *cppTemplates();
		static CppThread *cppThreads();
	};

	// Invalid size.
	const CppSize CppSize::invalid = { -1, -1, -1, -1 };

	// Invalid offset.
	const CppOffset CppOffset::invalid = { -1, -1 };

		/**
	 * Constructor for built-in classes.
	 */
	template <class T>
	void create1(void *mem) {
		new (mem)T();
	}

	template <class T, class P>
	void create2(void *mem, P p) {
		new (mem)T(p);
	}

	template <class T, class P, class Q>
	void create3(void *mem, P p, Q q) {
		new (mem)T(p, q);
	}

	template <class T, class P, class Q, class R>
	void create4(void *mem, P p, Q q, R r) {
		new (mem)T(p, q, r);
	}

	template <class T, class P, class Q, class R, class S>
	void create5(void *mem, P p, Q q, R r, S s) {
		new (mem)T(p, q, r, s);
	}

	template <class T, class P, class Q, class R, class S, class U>
	void create6(void *mem, P p, Q q, R r, S s, U u) {
		new (mem)T(p, q, r, s, u);
	}

	template <class T, class P, class Q, class R, class S, class U, class V>
	void create7(void *mem, P p, Q q, R r, S s, U u, V v) {
		new (mem)T(p, q, r, s, u, v);
	}

	// Wrap a destructor call.
	template <class T>
	void destroy(T *obj) {
		obj->~T();
	}

	CppType *CppMeta::cppTypes() {
		// PTR_OFFSETS

		static CppType types[] = {
			// CPP_TYPES
			{ null, null, CppType::superNone, 0, CppSize::invalid, null, typeNone, null },
		};
		return types;
	}

	CppFunction *CppMeta::cppFunctions() {
		// FN_PARAMETERS

		static CppFunction functions[] = {
			// CPP_FUNCTIONS
			{ null, null, CppFunction::fnFree, 0, null, null },
		};

		return functions;
	}

	CppTemplate *CppMeta::cppTemplates() {
		static CppTemplate templates[] = {
			// CPP_TEMPLATES
			{ null, null, null },
		};
		return templates;
	}

	CppThread *CppMeta::cppThreads() {
		static CppThread threads[] = {
			// CPP_THREADS
			{ null, null, null },
		};
		return threads;
	}

	const CppWorld *cppWorld() {
		static CppWorld w = {
			CppMeta::cppTypes(),
			CppMeta::cppFunctions(),
			CppMeta::cppTemplates(),
			CppMeta::cppThreads(),
		};
		return &w;
	}

}

#pragma optimize ("", off)
