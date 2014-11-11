#include "stdafx.h"
#include "BuiltIn.h"
#include "Str.h"
#include <stdarg.h>

// Below are auto-generated includes.
// BEGIN INCLUDES
#include "BnfReader.h"
#include "Lang/Simple.h"
#include "Lib/Object.h"
#include "Lib/Str.h"
#include "PkgReader.h"
#include "Test/VTest.h"
// END INCLUDES

// BEGIN STATIC
storm::Type *storm::Object::type(Engine &e) { return e.builtIn(0); }
storm::Type *storm::Object::type(Object *o) { return type(o->myType->engine); }
extern "C" void *cppVTable_storm__Object();
void *storm::Object::cppVTable() { return cppVTable_storm__Object(); }
storm::Type *storm::PkgFiles::type(Engine &e) { return e.builtIn(1); }
storm::Type *storm::PkgFiles::type(Object *o) { return type(o->myType->engine); }
extern "C" void *cppVTable_storm__PkgFiles();
void *storm::PkgFiles::cppVTable() { return cppVTable_storm__PkgFiles(); }
storm::Type *storm::PkgReader::type(Engine &e) { return e.builtIn(2); }
storm::Type *storm::PkgReader::type(Object *o) { return type(o->myType->engine); }
extern "C" void *cppVTable_storm__PkgReader();
void *storm::PkgReader::cppVTable() { return cppVTable_storm__PkgReader(); }
storm::Type *storm::SExpr::type(Engine &e) { return e.builtIn(3); }
storm::Type *storm::SExpr::type(Object *o) { return type(o->myType->engine); }
extern "C" void *cppVTable_storm__SExpr();
void *storm::SExpr::cppVTable() { return cppVTable_storm__SExpr(); }
storm::Type *storm::SScope::type(Engine &e) { return e.builtIn(4); }
storm::Type *storm::SScope::type(Object *o) { return type(o->myType->engine); }
extern "C" void *cppVTable_storm__SScope();
void *storm::SScope::cppVTable() { return cppVTable_storm__SScope(); }
storm::Type *storm::Str::type(Engine &e) { return e.builtIn(5); }
storm::Type *storm::Str::type(Object *o) { return type(o->myType->engine); }
extern "C" void *cppVTable_storm__Str();
void *storm::Str::cppVTable() { return cppVTable_storm__Str(); }
storm::Type *storm::VTest::type(Engine &e) { return e.builtIn(6); }
storm::Type *storm::VTest::type(Object *o) { return type(o->myType->engine); }
extern "C" void *cppVTable_storm__VTest();
void *storm::VTest::cppVTable() { return cppVTable_storm__VTest(); }
storm::Type *storm::bnf::Reader::type(Engine &e) { return e.builtIn(7); }
storm::Type *storm::bnf::Reader::type(Object *o) { return type(o->myType->engine); }
extern "C" void *cppVTable_storm__bnf__Reader();
void *storm::bnf::Reader::cppVTable() { return cppVTable_storm__bnf__Reader(); }
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
		return new (type)T();
	}

	template <class T, class P>
	T *create2(Type *type, P p) {
		return new (type)T(p);
	}

	template <class T, class P, class Q>
	T *create3(Type *type, P p, Q q) {
		return new (type)T(p, q);
	}

	/**
	 * Everything between BEGIN TYPES and END TYPES is auto-generated.
	 */
	const BuiltInType *builtInTypes() {
		static BuiltInType types[] = {
			// BEGIN TYPES
			{ Name(L"core"), L"Object", Name(), sizeof(storm::Object), 0 },
			{ Name(L"lang"), L"PkgFiles", Name(L"core.Object"), sizeof(storm::PkgFiles), 1 },
			{ Name(L"lang"), L"PkgReader", Name(L"core.Object"), sizeof(storm::PkgReader), 2 },
			{ Name(L"lang.simple"), L"SExpr", Name(L"core.Object"), sizeof(storm::SExpr), 3 },
			{ Name(L"lang.simple"), L"SScope", Name(L"core.Object"), sizeof(storm::SScope), 4 },
			{ Name(L"core"), L"Str", Name(L"core.Object"), sizeof(storm::Str), 5 },
			{ Name(L""), L"VTest", Name(L"core.Object"), sizeof(storm::VTest), 6 },
			{ Name(L"lang.bnf"), L"Reader", Name(L"lang.PkgReader"), sizeof(storm::bnf::Reader), 7 },
			// END TYPES
			{ L"", null, L"", null },
		};
		return types;
	}

	/**
	 * Everything between BEGIN LIST and END LIST is auto-generated.
	 */
	const BuiltInFunction *builtInFunctions() {
		static BuiltInFunction fns[] = {
			// BEGIN LIST
			{ Name(L"lang.bnf"), L"Reader", Name(L"lang.bnf.Reader"), L"__ctor", list(2, Name(L"core.Type"), Name(L"lang.PkgFiles")), address(&create2<storm::bnf::Reader, storm::PkgFiles *>) },
			{ Name(L"lang.simple"), L"SScope", Name(L"lang.simple.SScope"), L"__ctor", list(1, Name(L"core.Type")), address(&create1<storm::SScope>) },
			{ Name(L"lang.simple"), L"SScope", Name(), L"expr", list(1, Name(L"lang.simple.SExpr")), address<void(CODECALL storm::SScope::*)(storm::SExpr *)>(&storm::SScope::expr) },
			{ Name(L"lang.simple"), null, Name(L"lang.simple.SExpr"), L"sOperator", list(3, Name(L"lang.simple.SExpr"), Name(L"lang.simple.SExpr"), Name(L"core.Str")), address<storm::SExpr *(CODECALL *)(storm::SExpr *, storm::SExpr *, storm::Str *)>(&storm::sOperator) },
			{ Name(L"lang.simple"), null, Name(L"lang.simple.SExpr"), L"sVar", list(1, Name(L"core.Str")), address<storm::SExpr *(CODECALL *)(storm::Str *)>(&storm::sVar) },
			{ Name(L"lang.simple"), null, Name(L"lang.simple.SExpr"), L"sNr", list(1, Name(L"core.Str")), address<storm::SExpr *(CODECALL *)(storm::Str *)>(&storm::sNr) },
			{ Name(L"core"), L"Object", Name(L"core.Str"), L"toS", list(0), address<storm::Str *(CODECALL storm::Object::*)()>(&storm::Object::toS) },
			{ Name(L"core"), L"Object", Name(L"core.Bool"), L"equals", list(1, Name(L"core.Object")), address<Bool(CODECALL storm::Object::*)(storm::Object *)>(&storm::Object::equals) },
			{ Name(L"core"), L"Str", Name(L"core.Str"), L"__ctor", list(1, Name(L"core.Type")), address(&create1<storm::Str>) },
			{ Name(L"core"), L"Str", Name(L"core.Str"), L"__ctor", list(2, Name(L"core.Type"), Name(L"core.Str")), address(&create2<storm::Str, const storm::Str &>) },
			{ Name(L"core"), L"Str", Name(L"core.Nat"), L"count", list(0), address<Nat(CODECALL storm::Str::*)() const>(&storm::Str::count) },
			{ Name(L"core"), L"Str", Name(L"core.Bool"), L"equals", list(1, Name(L"core.Object")), address<Bool(CODECALL storm::Str::*)(storm::Object *)>(&storm::Str::equals) },
			{ Name(L"core"), L"Str", Name(L"core.Str"), L"toS", list(0), address<storm::Str *(CODECALL storm::Str::*)()>(&storm::Str::toS) },
			{ Name(L"lang"), L"PkgFiles", Name(L"lang.PkgFiles"), L"__ctor", list(1, Name(L"core.Type")), address(&create1<storm::PkgFiles>) },
			{ Name(L"lang"), L"PkgFiles", Name(L"core.Str"), L"toS", list(0), address<storm::Str *(CODECALL storm::PkgFiles::*)()>(&storm::PkgFiles::toS) },
			{ Name(L"lang"), L"PkgReader", Name(L"lang.PkgReader"), L"__ctor", list(2, Name(L"core.Type"), Name(L"lang.PkgFiles")), address(&create2<storm::PkgReader, storm::PkgFiles *>) },
			{ Name(L""), L"VTest", Name(L"VTest"), L"__ctor", list(1, Name(L"core.Type")), address(&create1<storm::VTest>) },
			{ Name(L""), L"VTest", Name(L"core.Int"), L"returnOne", list(0), address<Int(CODECALL storm::VTest::*)()>(&storm::VTest::returnOne) },
			{ Name(L""), L"VTest", Name(L"core.Int"), L"returnTwo", list(0), address<Int(CODECALL storm::VTest::*)()>(&storm::VTest::returnTwo) },
			// END LIST
			{ Name(), null, Name(), L"", list(0), null },
		};
		return fns;
	}

}