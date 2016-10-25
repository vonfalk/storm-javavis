#include "stdafx.h"
#include "Utils/Memory.h"
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

// VTABLE_DECLS

// Turn off optimizations in this file. It takes quite a long time, and since it is only executed
// once during compiler startup, it is not very useful to optimize these functions. Especially not
// during testing!
#pragma optimize("", off)

namespace storm {

	struct CppMeta {
		static const CppType *cppTypes();
		static const CppFunction *cppFunctions();
		static const CppTemplate *cppTemplates();
		static const CppThread *cppThreads();
		static const CppRefType *cppRefTypes();

		/**
		 * Constructor for built-in classes.
		 */
		template <class T>
		static void create1(void *mem) {
			new (Place(mem))T();
		}

		template <class T, class P>
		static void create2(void *mem, P p) {
			new (Place(mem))T(p);
		}

		template <class T, class P, class Q>
		static void create3(void *mem, P p, Q q) {
			new (Place(mem))T(p, q);
		}

		template <class T, class P, class Q, class R>
		static void create4(void *mem, P p, Q q, R r) {
			new (Place(mem))T(p, q, r);
		}

		template <class T, class P, class Q, class R, class S>
		static void create5(void *mem, P p, Q q, R r, S s) {
			new (Place(mem))T(p, q, r, s);
		}

		template <class T, class P, class Q, class R, class S, class U>
		static void create6(void *mem, P p, Q q, R r, S s, U u) {
			new (Place(mem))T(p, q, r, s, u);
		}

		template <class T, class P, class Q, class R, class S, class U, class V>
		static void create7(void *mem, P p, Q q, R r, S s, U u, V v) {
			new (Place(mem))T(p, q, r, s, u, v);
		}

		// Wrap a destructor call.
		template <class T>
		static void destroy(T *obj) {
			obj->~T();
		}
	};

	// Invalid size.
	const CppSize CppSize::invalid = { -1, -1, -1, -1 };

	// Invalid offset.
	const CppOffset CppOffset::invalid = { -1, -1 };

	const CppType *CppMeta::cppTypes() {
		// PTR_OFFSETS

		static const CppType types[] = {
			// CPP_TYPES
			{ null, null, CppType::superNone, 0, CppSize::invalid, null, typeNone, null },
		};
		return types;
	}

	const CppFunction *CppMeta::cppFunctions() {
		// FN_PARAMETERS

		static const CppFunction functions[] = {
			// CPP_FUNCTIONS
			{ null, null, CppFunction::fnFree, 0, null, null, null },
		};

		return functions;
	}

	const CppTemplate *CppMeta::cppTemplates() {
		static const CppTemplate templates[] = {
			// CPP_TEMPLATES
			{ null, null, null },
		};
		return templates;
	}

	const CppThread *CppMeta::cppThreads() {
		static const CppThread threads[] = {
			// CPP_THREADS
			{ null, null, null },
		};
		return threads;
	}

	const CppRefType *CppMeta::cppRefTypes() {
#ifdef DEBUG
		// REF_PTR_OFFSETS

		static const CppRefType types[] = {
			// REF_TYPES
			{ -1, null },
		};
		return types;
#else
		return null;
#endif
	}

	const CppWorld *cppWorld() {
		static CppWorld w = {
			CppMeta::cppTypes(),
			CppMeta::cppFunctions(),
			CppMeta::cppTemplates(),
			CppMeta::cppThreads(),
			CppMeta::cppRefTypes(),
		};
		return &w;
	}

}

#pragma optimize ("", off)
