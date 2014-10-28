#include "stdafx.h"
#include "BuiltIn.h"
#include "Str.h"
#include <stdarg.h>

// Below are auto-generated includes.
// BEGIN INCLUDES
#include "Lang/Simple.h"
#include "Lib/Str.h"
#include "Test/VTest.h"
// END INCLUDES

// BEGIN STATIC
storm::Type *storm::VTest::type(Engine &e) { return e.builtIn(0); }
storm::Type *storm::VTest::type(Object *o) { return type(o->myType->engine); }
extern "C" void *cppVTable_storm__VTest();
void *storm::VTest::cppVTable() { return cppVTable_storm__VTest(); }
storm::Type *storm::Str::type(Engine &e) { return e.builtIn(1); }
storm::Type *storm::Str::type(Object *o) { return type(o->myType->engine); }
extern "C" void *cppVTable_storm__Str();
void *storm::Str::cppVTable() { return cppVTable_storm__Str(); }
storm::Type *storm::SExpr::type(Engine &e) { return e.builtIn(2); }
storm::Type *storm::SExpr::type(Object *o) { return type(o->myType->engine); }
extern "C" void *cppVTable_storm__SExpr();
void *storm::SExpr::cppVTable() { return cppVTable_storm__SExpr(); }
storm::Type *storm::SScope::type(Engine &e) { return e.builtIn(3); }
storm::Type *storm::SScope::type(Object *o) { return type(o->myType->engine); }
extern "C" void *cppVTable_storm__SScope();
void *storm::SScope::cppVTable() { return cppVTable_storm__SScope(); }
// END STATIC

namespace storm {

	/**
	 * Create a vector from a argument list.
	 */
	vector<Name> list(nat count, ...) {
		va_list l;
		va_start(l, count);

		vector<Name> result;
		for (nat i = 0; i < count; i++)
			result.push_back(va_arg(l, Name));

		va_end(l);
		return result;
	}

	/**
	 * Constructor for built-in classes.
	 */
	template <class T>
	T *create1(Type *type) {
		return new T(type);
	}

	template <class T, class P>
	T *create2(Type *type, const P &p) {
		return new T(type, p);
	}

	/**
	 * Everything between BEGIN TYPES and END TYPES is auto-generated.
	 */
	const BuiltInType *builtInTypes() {
		static BuiltInType types[] = {
			// BEGIN TYPES
			{ Name(L""), L"VTest", null, 0 },
			{ Name(L"core"), L"Str", null, 1 },
			{ Name(L"lang.simple"), L"SExpr", null, 2 },
			{ Name(L"lang.simple"), L"SScope", null, 3 },
			// END TYPES
			{ L"", null, null, null },
		};
		return types;
	}

	/**
	 * Everything between BEGIN LIST and END LIST is auto-generated.
	 */
	const BuiltInFunction *builtInFunctions() {
		static BuiltInFunction fns[] = {
			// BEGIN LIST
			{ Name(L""), L"VTest", Name(L"VTest"), L"__ctor", list(1, Name(L"Type")), address(&create1<storm::VTest>) },
			{ Name(L""), L"VTest", Name(L""), L"returnOne", list(0), address(&storm::VTest::returnOne) },
			{ Name(L""), L"VTest", Name(L""), L"returnTwo", list(0), address(&storm::VTest::returnTwo) },
			{ Name(L"core"), L"Str", Name(L"Str"), L"__ctor", list(1, Name(L"Type")), address(&create1<storm::Str>) },
			{ Name(L"core"), L"Str", Name(L"Str"), L"__ctor", list(2, Name(L"Type"), Name(L"Str")), address(&create2<storm::Str, Str>) },
			{ Name(L"core"), L"Str", Name(L"Nat"), L"count", list(0), address(&storm::Str::count) },
			{ Name(L"lang.simple"), L"SScope", Name(L"SScope"), L"__ctor", list(1, Name(L"Type")), address(&create1<storm::SScope>) },
			{ Name(L"lang.simple"), L"SScope", Name(), L"expr", list(1, Name(L"SExpr")), address(&storm::SScope::expr) },
			{ Name(L"lang.simple"), null, Name(L"SExpr"), L"sNr", list(1, Name(L"Str")), address(&storm::sNr) },
			{ Name(L"lang.simple"), null, Name(L"SExpr"), L"sOperator", list(3, Name(L"SExpr"), Name(L"SExpr"), Name(L"Str")), address(&storm::sOperator) },
			{ Name(L"lang.simple"), null, Name(L"SExpr"), L"sVar", list(1, Name(L"Str")), address(&storm::sVar) },
			// END LIST
			{ Name(), null, Name(), L"", list(0), null },
		};
		return fns;
	}

}