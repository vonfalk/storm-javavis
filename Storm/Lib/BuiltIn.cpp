#include "stdafx.h"
#include "BuiltIn.h"
#include "Str.h"
#include "Type.h"
#include <stdarg.h>

// Below are auto-generated includes.
// BEGIN INCLUDES
#include "Basic/BSClass.h"
#include "Basic/BSContents.h"
#include "Basic/BSFunction.h"
#include "Basic/BSIncludes.h"
#include "Basic/BSParams.h"
#include "Basic/BSPkg.h"
#include "Basic/BSReader.h"
#include "Basic/BSType.h"
#include "BnfReader.h"
#include "Code.h"
#include "Function.h"
#include "Lang/Simple.h"
#include "Lib/BoolDef.h"
#include "Lib/Int.h"
#include "Lib/Object.h"
#include "Lib/Str.h"
#include "Named.h"
#include "Overload.h"
#include "Package.h"
#include "PkgReader.h"
#include "SrcPos.h"
#include "SyntaxObject.h"
#include "Test/VTest.h"
#include "Type.h"
// END INCLUDES

// BEGIN STATIC
storm::Type *storm::Type::type(Engine &e) { return e.builtIn(0); }
storm::Type *storm::Type::type(Object *o) { return type(o->myType->engine); }
extern "C" void *cppVTable_storm__Type();
void *storm::Type::cppVTable() { return cppVTable_storm__Type(); }
storm::Type *storm::BoolType::type(Engine &e) { return e.builtIn(1); }
storm::Type *storm::BoolType::type(Object *o) { return type(o->myType->engine); }
extern "C" void *cppVTable_storm__BoolType();
void *storm::BoolType::cppVTable() { return cppVTable_storm__BoolType(); }
storm::Type *storm::Code::type(Engine &e) { return e.builtIn(2); }
storm::Type *storm::Code::type(Object *o) { return type(o->myType->engine); }
extern "C" void *cppVTable_storm__Code();
void *storm::Code::cppVTable() { return cppVTable_storm__Code(); }
storm::Type *storm::DelegatedCode::type(Engine &e) { return e.builtIn(3); }
storm::Type *storm::DelegatedCode::type(Object *o) { return type(o->myType->engine); }
extern "C" void *cppVTable_storm__DelegatedCode();
void *storm::DelegatedCode::cppVTable() { return cppVTable_storm__DelegatedCode(); }
storm::Type *storm::FileReader::type(Engine &e) { return e.builtIn(4); }
storm::Type *storm::FileReader::type(Object *o) { return type(o->myType->engine); }
extern "C" void *cppVTable_storm__FileReader();
void *storm::FileReader::cppVTable() { return cppVTable_storm__FileReader(); }
storm::Type *storm::FilesReader::type(Engine &e) { return e.builtIn(5); }
storm::Type *storm::FilesReader::type(Object *o) { return type(o->myType->engine); }
extern "C" void *cppVTable_storm__FilesReader();
void *storm::FilesReader::cppVTable() { return cppVTable_storm__FilesReader(); }
storm::Type *storm::Function::type(Engine &e) { return e.builtIn(6); }
storm::Type *storm::Function::type(Object *o) { return type(o->myType->engine); }
extern "C" void *cppVTable_storm__Function();
void *storm::Function::cppVTable() { return cppVTable_storm__Function(); }
storm::Type *storm::IntType::type(Engine &e) { return e.builtIn(7); }
storm::Type *storm::IntType::type(Object *o) { return type(o->myType->engine); }
extern "C" void *cppVTable_storm__IntType();
void *storm::IntType::cppVTable() { return cppVTable_storm__IntType(); }
storm::Type *storm::LazyCode::type(Engine &e) { return e.builtIn(8); }
storm::Type *storm::LazyCode::type(Object *o) { return type(o->myType->engine); }
extern "C" void *cppVTable_storm__LazyCode();
void *storm::LazyCode::cppVTable() { return cppVTable_storm__LazyCode(); }
storm::Type *storm::NameLookup::type(Engine &e) { return e.builtIn(9); }
storm::Type *storm::NameLookup::type(Object *o) { return type(o->myType->engine); }
extern "C" void *cppVTable_storm__NameLookup();
void *storm::NameLookup::cppVTable() { return cppVTable_storm__NameLookup(); }
storm::Type *storm::NameOverload::type(Engine &e) { return e.builtIn(10); }
storm::Type *storm::NameOverload::type(Object *o) { return type(o->myType->engine); }
extern "C" void *cppVTable_storm__NameOverload();
void *storm::NameOverload::cppVTable() { return cppVTable_storm__NameOverload(); }
storm::Type *storm::Named::type(Engine &e) { return e.builtIn(11); }
storm::Type *storm::Named::type(Object *o) { return type(o->myType->engine); }
extern "C" void *cppVTable_storm__Named();
void *storm::Named::cppVTable() { return cppVTable_storm__Named(); }
storm::Type *storm::NatType::type(Engine &e) { return e.builtIn(12); }
storm::Type *storm::NatType::type(Object *o) { return type(o->myType->engine); }
extern "C" void *cppVTable_storm__NatType();
void *storm::NatType::cppVTable() { return cppVTable_storm__NatType(); }
storm::Type *storm::Object::type(Engine &e) { return e.builtIn(13); }
storm::Type *storm::Object::type(Object *o) { return type(o->myType->engine); }
extern "C" void *cppVTable_storm__Object();
void *storm::Object::cppVTable() { return cppVTable_storm__Object(); }
storm::Type *storm::Overload::type(Engine &e) { return e.builtIn(14); }
storm::Type *storm::Overload::type(Object *o) { return type(o->myType->engine); }
extern "C" void *cppVTable_storm__Overload();
void *storm::Overload::cppVTable() { return cppVTable_storm__Overload(); }
storm::Type *storm::Package::type(Engine &e) { return e.builtIn(15); }
storm::Type *storm::Package::type(Object *o) { return type(o->myType->engine); }
extern "C" void *cppVTable_storm__Package();
void *storm::Package::cppVTable() { return cppVTable_storm__Package(); }
storm::Type *storm::PkgFiles::type(Engine &e) { return e.builtIn(16); }
storm::Type *storm::PkgFiles::type(Object *o) { return type(o->myType->engine); }
extern "C" void *cppVTable_storm__PkgFiles();
void *storm::PkgFiles::cppVTable() { return cppVTable_storm__PkgFiles(); }
storm::Type *storm::PkgReader::type(Engine &e) { return e.builtIn(17); }
storm::Type *storm::PkgReader::type(Object *o) { return type(o->myType->engine); }
extern "C" void *cppVTable_storm__PkgReader();
void *storm::PkgReader::cppVTable() { return cppVTable_storm__PkgReader(); }
storm::Type *storm::SExpr::type(Engine &e) { return e.builtIn(18); }
storm::Type *storm::SExpr::type(Object *o) { return type(o->myType->engine); }
extern "C" void *cppVTable_storm__SExpr();
void *storm::SExpr::cppVTable() { return cppVTable_storm__SExpr(); }
storm::Type *storm::SObject::type(Engine &e) { return e.builtIn(19); }
storm::Type *storm::SObject::type(Object *o) { return type(o->myType->engine); }
extern "C" void *cppVTable_storm__SObject();
void *storm::SObject::cppVTable() { return cppVTable_storm__SObject(); }
storm::Type *storm::SScope::type(Engine &e) { return e.builtIn(20); }
storm::Type *storm::SScope::type(Object *o) { return type(o->myType->engine); }
extern "C" void *cppVTable_storm__SScope();
void *storm::SScope::cppVTable() { return cppVTable_storm__SScope(); }
storm::Type *storm::SStr::type(Engine &e) { return e.builtIn(21); }
storm::Type *storm::SStr::type(Object *o) { return type(o->myType->engine); }
extern "C" void *cppVTable_storm__SStr();
void *storm::SStr::cppVTable() { return cppVTable_storm__SStr(); }
storm::Type *storm::SrcPos::type(Engine &e) { return e.builtIn(22); }
storm::Type *storm::SrcPos::type(Object *o) { return type(o->myType->engine); }
storm::Type *storm::StaticCode::type(Engine &e) { return e.builtIn(23); }
storm::Type *storm::StaticCode::type(Object *o) { return type(o->myType->engine); }
extern "C" void *cppVTable_storm__StaticCode();
void *storm::StaticCode::cppVTable() { return cppVTable_storm__StaticCode(); }
storm::Type *storm::Str::type(Engine &e) { return e.builtIn(24); }
storm::Type *storm::Str::type(Object *o) { return type(o->myType->engine); }
extern "C" void *cppVTable_storm__Str();
void *storm::Str::cppVTable() { return cppVTable_storm__Str(); }
storm::Type *storm::VTest::type(Engine &e) { return e.builtIn(25); }
storm::Type *storm::VTest::type(Object *o) { return type(o->myType->engine); }
extern "C" void *cppVTable_storm__VTest();
void *storm::VTest::cppVTable() { return cppVTable_storm__VTest(); }
storm::Type *storm::bnf::Reader::type(Engine &e) { return e.builtIn(26); }
storm::Type *storm::bnf::Reader::type(Object *o) { return type(o->myType->engine); }
extern "C" void *cppVTable_storm__bnf__Reader();
void *storm::bnf::Reader::cppVTable() { return cppVTable_storm__bnf__Reader(); }
storm::Type *storm::bs::BSFunction::type(Engine &e) { return e.builtIn(27); }
storm::Type *storm::bs::BSFunction::type(Object *o) { return type(o->myType->engine); }
extern "C" void *cppVTable_storm__bs__BSFunction();
void *storm::bs::BSFunction::cppVTable() { return cppVTable_storm__bs__BSFunction(); }
storm::Type *storm::bs::Class::type(Engine &e) { return e.builtIn(28); }
storm::Type *storm::bs::Class::type(Object *o) { return type(o->myType->engine); }
extern "C" void *cppVTable_storm__bs__Class();
void *storm::bs::Class::cppVTable() { return cppVTable_storm__bs__Class(); }
storm::Type *storm::bs::Contents::type(Engine &e) { return e.builtIn(29); }
storm::Type *storm::bs::Contents::type(Object *o) { return type(o->myType->engine); }
extern "C" void *cppVTable_storm__bs__Contents();
void *storm::bs::Contents::cppVTable() { return cppVTable_storm__bs__Contents(); }
storm::Type *storm::bs::File::type(Engine &e) { return e.builtIn(30); }
storm::Type *storm::bs::File::type(Object *o) { return type(o->myType->engine); }
extern "C" void *cppVTable_storm__bs__File();
void *storm::bs::File::cppVTable() { return cppVTable_storm__bs__File(); }
storm::Type *storm::bs::FunctionDecl::type(Engine &e) { return e.builtIn(31); }
storm::Type *storm::bs::FunctionDecl::type(Object *o) { return type(o->myType->engine); }
extern "C" void *cppVTable_storm__bs__FunctionDecl();
void *storm::bs::FunctionDecl::cppVTable() { return cppVTable_storm__bs__FunctionDecl(); }
storm::Type *storm::bs::Includes::type(Engine &e) { return e.builtIn(32); }
storm::Type *storm::bs::Includes::type(Object *o) { return type(o->myType->engine); }
extern "C" void *cppVTable_storm__bs__Includes();
void *storm::bs::Includes::cppVTable() { return cppVTable_storm__bs__Includes(); }
storm::Type *storm::bs::Params::type(Engine &e) { return e.builtIn(33); }
storm::Type *storm::bs::Params::type(Object *o) { return type(o->myType->engine); }
extern "C" void *cppVTable_storm__bs__Params();
void *storm::bs::Params::cppVTable() { return cppVTable_storm__bs__Params(); }
storm::Type *storm::bs::Pkg::type(Engine &e) { return e.builtIn(34); }
storm::Type *storm::bs::Pkg::type(Object *o) { return type(o->myType->engine); }
extern "C" void *cppVTable_storm__bs__Pkg();
void *storm::bs::Pkg::cppVTable() { return cppVTable_storm__bs__Pkg(); }
storm::Type *storm::bs::PkgName::type(Engine &e) { return e.builtIn(35); }
storm::Type *storm::bs::PkgName::type(Object *o) { return type(o->myType->engine); }
extern "C" void *cppVTable_storm__bs__PkgName();
void *storm::bs::PkgName::cppVTable() { return cppVTable_storm__bs__PkgName(); }
storm::Type *storm::bs::Reader::type(Engine &e) { return e.builtIn(36); }
storm::Type *storm::bs::Reader::type(Object *o) { return type(o->myType->engine); }
extern "C" void *cppVTable_storm__bs__Reader();
void *storm::bs::Reader::cppVTable() { return cppVTable_storm__bs__Reader(); }
storm::Type *storm::bs::TypeName::type(Engine &e) { return e.builtIn(37); }
storm::Type *storm::bs::TypeName::type(Object *o) { return type(o->myType->engine); }
extern "C" void *cppVTable_storm__bs__TypeName();
void *storm::bs::TypeName::cppVTable() { return cppVTable_storm__bs__TypeName(); }
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

	template <class T, class P, class Q, class R>
	T *create4(Type *type, P p, Q q, R r) {
		return new (type)T(p, q, r);
	}

	template <class T, class P, class Q, class R, class S>
	T *create5(Type *type, P p, Q q, R r, S s) {
		return new (type)T(p, q, r, s);
	}

	template <class T, class P, class Q, class R, class S, class U>
	T *create6(Type *type, P p, Q q, R r, S s, U u) {
		return new (type)T(p, q, r, s, u);
	}

	/**
	 * Everything between BEGIN TYPES and END TYPES is auto-generated.
	 */
	const BuiltInType *builtInTypes() {
		static BuiltInType types[] = {
			// BEGIN TYPES
			{ Name(L""), L"Type", Name(L"core.lang.Named"), sizeof(storm::Type), typeClass, 0 },
			{ Name(L""), L"BoolType", Name(L"Type"), sizeof(storm::BoolType), typeClass, 1 },
			{ Name(L"core.lang"), L"Code", Name(L"core.Object"), sizeof(storm::Code), typeClass, 2 },
			{ Name(L"core.lang"), L"DelegatedCode", Name(L"core.lang.Code"), sizeof(storm::DelegatedCode), typeClass, 3 },
			{ Name(L"lang"), L"FileReader", Name(L"core.Object"), sizeof(storm::FileReader), typeClass, 4 },
			{ Name(L"lang"), L"FilesReader", Name(L"lang.PkgReader"), sizeof(storm::FilesReader), typeClass, 5 },
			{ Name(L"core.lang"), L"Function", Name(L"NameOverload"), sizeof(storm::Function), typeClass, 6 },
			{ Name(L""), L"IntType", Name(L"Type"), sizeof(storm::IntType), typeClass, 7 },
			{ Name(L"core.lang"), L"LazyCode", Name(L"core.lang.Code"), sizeof(storm::LazyCode), typeClass, 8 },
			{ Name(L"core.lang"), L"NameLookup", Name(L"core.Object"), sizeof(storm::NameLookup), typeClass, 9 },
			{ Name(L""), L"NameOverload", Name(L"core.lang.Named"), sizeof(storm::NameOverload), typeClass, 10 },
			{ Name(L"core.lang"), L"Named", Name(L"core.lang.NameLookup"), sizeof(storm::Named), typeClass, 11 },
			{ Name(L""), L"NatType", Name(L"Type"), sizeof(storm::NatType), typeClass, 12 },
			{ Name(L"core"), L"Object", Name(), sizeof(storm::Object), typeClass, 13 },
			{ Name(L""), L"Overload", Name(L"core.lang.Named"), sizeof(storm::Overload), typeClass, 14 },
			{ Name(L""), L"Package", Name(L"core.lang.Named"), sizeof(storm::Package), typeClass, 15 },
			{ Name(L"lang"), L"PkgFiles", Name(L"core.Object"), sizeof(storm::PkgFiles), typeClass, 16 },
			{ Name(L"lang"), L"PkgReader", Name(L"core.Object"), sizeof(storm::PkgReader), typeClass, 17 },
			{ Name(L"lang.simple"), L"SExpr", Name(L"core.lang.SObject"), sizeof(storm::SExpr), typeClass, 18 },
			{ Name(L"core.lang"), L"SObject", Name(L"core.Object"), sizeof(storm::SObject), typeClass, 19 },
			{ Name(L"lang.simple"), L"SScope", Name(L"core.lang.SObject"), sizeof(storm::SScope), typeClass, 20 },
			{ Name(L"core.lang"), L"SStr", Name(L"core.lang.SObject"), sizeof(storm::SStr), typeClass, 21 },
			{ Name(L"core.lang"), L"SrcPos", Name(), sizeof(storm::SrcPos), typeValue, 22 },
			{ Name(L"core.lang"), L"StaticCode", Name(L"core.lang.Code"), sizeof(storm::StaticCode), typeClass, 23 },
			{ Name(L"core"), L"Str", Name(L"core.Object"), sizeof(storm::Str), typeClass, 24 },
			{ Name(L""), L"VTest", Name(L"core.Object"), sizeof(storm::VTest), typeClass, 25 },
			{ Name(L"lang.bnf"), L"Reader", Name(L"lang.PkgReader"), sizeof(storm::bnf::Reader), typeClass, 26 },
			{ Name(L""), L"BSFunction", Name(L"core.lang.Function"), sizeof(storm::bs::BSFunction), typeClass, 27 },
			{ Name(L"lang.bs"), L"Class", Name(L"Type"), sizeof(storm::bs::Class), typeClass, 28 },
			{ Name(L"lang.bs"), L"Contents", Name(L"core.Object"), sizeof(storm::bs::Contents), typeClass, 29 },
			{ Name(L"lang.bs"), L"File", Name(L"lang.FileReader"), sizeof(storm::bs::File), typeClass, 30 },
			{ Name(L""), L"FunctionDecl", Name(L"core.Object"), sizeof(storm::bs::FunctionDecl), typeClass, 31 },
			{ Name(L"lang.bs"), L"Includes", Name(L"core.lang.SObject"), sizeof(storm::bs::Includes), typeClass, 32 },
			{ Name(L""), L"Params", Name(L"core.Object"), sizeof(storm::bs::Params), typeClass, 33 },
			{ Name(L"lang.bs"), L"Pkg", Name(L"core.lang.SObject"), sizeof(storm::bs::Pkg), typeClass, 34 },
			{ Name(L""), L"PkgName", Name(L"core.lang.SObject"), sizeof(storm::bs::PkgName), typeClass, 35 },
			{ Name(L"lang.bs"), L"Reader", Name(L"lang.FilesReader"), sizeof(storm::bs::Reader), typeClass, 36 },
			{ Name(L""), L"TypeName", Name(L"core.lang.SObject"), sizeof(storm::bs::TypeName), typeClass, 37 },
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
			{ Name(L"lang.bs"), L"Class", Name(L"lang.bs.Class"), L"__ctor", list(4, Name(L"Type"), Name(L"core.lang.SrcPos"), Name(L"core.lang.SStr"), Name(L"core.lang.SStr")), address(&create4<storm::bs::Class, storm::SrcPos, storm::SStr *, storm::SStr *>) },
			{ Name(L"lang.bs"), L"Contents", Name(L"lang.bs.Contents"), L"__ctor", list(1, Name(L"Type")), address(&create1<storm::bs::Contents>) },
			{ Name(L"lang.bs"), L"Contents", Name(), L"add", list(1, Name(L"Type")), address<void(CODECALL storm::bs::Contents::*)(storm::Auto<storm::Type>)>(&storm::bs::Contents::add) },
			{ Name(L"lang.bs"), L"Contents", Name(), L"add", list(1, Name(L"FunctionDecl")), address<void(CODECALL storm::bs::Contents::*)(storm::Auto<storm::bs::FunctionDecl>)>(&storm::bs::Contents::add) },
			{ Name(L""), L"FunctionDecl", Name(L"FunctionDecl"), L"__ctor", list(6, Name(L"Type"), Name(L"core.lang.SrcPos"), Name(L"core.lang.SStr"), Name(L"TypeName"), Name(L"Params"), Name(L"core.lang.SStr")), address(&create6<storm::bs::FunctionDecl, storm::SrcPos, storm::SStr *, storm::bs::TypeName *, storm::bs::Params *, storm::SStr *>) },
			{ Name(L"lang.bs"), L"Includes", Name(L"lang.bs.Includes"), L"__ctor", list(1, Name(L"Type")), address(&create1<storm::bs::Includes>) },
			{ Name(L"lang.bs"), L"Includes", Name(), L"add", list(1, Name(L"lang.bs.Pkg")), address<void(CODECALL storm::bs::Includes::*)(storm::Auto<storm::bs::Pkg>)>(&storm::bs::Includes::add) },
			{ Name(L""), L"Params", Name(L"Params"), L"__ctor", list(1, Name(L"Type")), address(&create1<storm::bs::Params>) },
			{ Name(L""), L"Params", Name(), L"add", list(1, Name(L"TypeName")), address<void(CODECALL storm::bs::Params::*)(storm::Auto<storm::bs::TypeName>)>(&storm::bs::Params::add) },
			{ Name(L"lang.bs"), L"Pkg", Name(L"lang.bs.Pkg"), L"__ctor", list(1, Name(L"Type")), address(&create1<storm::bs::Pkg>) },
			{ Name(L"lang.bs"), L"Pkg", Name(), L"add", list(1, Name(L"core.lang.SStr")), address<void(CODECALL storm::bs::Pkg::*)(storm::Auto<storm::SStr>)>(&storm::bs::Pkg::add) },
			{ Name(L"lang.bs"), L"Reader", Name(L"lang.bs.Reader"), L"__ctor", list(3, Name(L"Type"), Name(L"lang.PkgFiles"), Name(L"Package")), address(&create3<storm::bs::Reader, storm::PkgFiles *, storm::Package *>) },
			{ Name(L""), L"PkgName", Name(L"PkgName"), L"__ctor", list(1, Name(L"Type")), address(&create1<storm::bs::PkgName>) },
			{ Name(L""), L"PkgName", Name(), L"add", list(1, Name(L"core.lang.SStr")), address<void(CODECALL storm::bs::PkgName::*)(storm::Auto<storm::SStr>)>(&storm::bs::PkgName::add) },
			{ Name(L""), L"TypeName", Name(L"TypeName"), L"__ctor", list(2, Name(L"Type"), Name(L"core.lang.SStr")), address(&create2<storm::bs::TypeName, storm::SStr *>) },
			{ Name(L""), L"TypeName", Name(L"TypeName"), L"__ctor", list(3, Name(L"Type"), Name(L"PkgName"), Name(L"core.lang.SStr")), address(&create3<storm::bs::TypeName, storm::bs::PkgName *, storm::SStr *>) },
			{ Name(L"lang.bnf"), L"Reader", Name(L"lang.bnf.Reader"), L"__ctor", list(3, Name(L"Type"), Name(L"lang.PkgFiles"), Name(L"Package")), address(&create3<storm::bnf::Reader, storm::PkgFiles *, storm::Package *>) },
			{ Name(L"core.lang"), L"LazyCode", Name(L"core.lang.LazyCode"), L"__ctor", list(1, Name(L"Type")), address(&create1<storm::LazyCode>) },
			{ Name(L"lang.simple"), L"SScope", Name(L"lang.simple.SScope"), L"__ctor", list(1, Name(L"Type")), address(&create1<storm::SScope>) },
			{ Name(L"lang.simple"), L"SScope", Name(), L"expr", list(1, Name(L"lang.simple.SExpr")), address<void(CODECALL storm::SScope::*)(storm::Auto<storm::SExpr>)>(&storm::SScope::expr) },
			{ Name(L"lang.simple"), null, Name(L"lang.simple.SExpr"), L"sOperator", list(3, Name(L"lang.simple.SExpr"), Name(L"lang.simple.SExpr"), Name(L"core.lang.SStr")), address<storm::SExpr *(CODECALL *)(storm::Auto<storm::SExpr>, storm::Auto<storm::SExpr>, storm::Auto<storm::SStr>)>(&storm::sOperator) },
			{ Name(L"lang.simple"), null, Name(L"lang.simple.SExpr"), L"sVar", list(1, Name(L"core.lang.SStr")), address<storm::SExpr *(CODECALL *)(storm::Auto<storm::SStr>)>(&storm::sVar) },
			{ Name(L"lang.simple"), null, Name(L"lang.simple.SExpr"), L"sNr", list(1, Name(L"core.lang.SStr")), address<storm::SExpr *(CODECALL *)(storm::Auto<storm::SStr>)>(&storm::sNr) },
			{ Name(L"core"), L"Object", Name(L"core.Str"), L"toS", list(0), address<storm::Str *(CODECALL storm::Object::*)()>(&storm::Object::toS) },
			{ Name(L"core"), L"Object", Name(L"core.Bool"), L"equals", list(1, Name(L"core.Object")), address<Bool(CODECALL storm::Object::*)(storm::Auto<storm::Object>)>(&storm::Object::equals) },
			{ Name(L"core"), L"Str", Name(L"core.Str"), L"__ctor", list(1, Name(L"Type")), address(&create1<storm::Str>) },
			{ Name(L"core"), L"Str", Name(L"core.Str"), L"__ctor", list(2, Name(L"Type"), Name(L"core.Str")), address(&create2<storm::Str, storm::Str *>) },
			{ Name(L"core"), L"Str", Name(L"core.Nat"), L"count", list(0), address<Nat(CODECALL storm::Str::*)() const>(&storm::Str::count) },
			{ Name(L"core"), L"Str", Name(L"core.Bool"), L"equals", list(1, Name(L"core.Object")), address<Bool(CODECALL storm::Str::*)(storm::Auto<storm::Object>)>(&storm::Str::equals) },
			{ Name(L"core"), L"Str", Name(L"core.Str"), L"toS", list(0), address<storm::Str *(CODECALL storm::Str::*)()>(&storm::Str::toS) },
			{ Name(L"core.lang"), L"Named", Name(L"core.lang.Named"), L"__ctor", list(2, Name(L"Type"), Name(L"core.Str")), address(&create2<storm::Named, storm::Str *>) },
			{ Name(L""), L"Package", Name(), L"add", list(1, Name(L"Package")), address<void(CODECALL storm::Package::*)(storm::Auto<storm::Package>)>(&storm::Package::add) },
			{ Name(L""), L"Package", Name(), L"add", list(1, Name(L"Type")), address<void(CODECALL storm::Package::*)(storm::Auto<storm::Type>)>(&storm::Package::add) },
			{ Name(L"lang"), L"PkgFiles", Name(L"lang.PkgFiles"), L"__ctor", list(1, Name(L"Type")), address(&create1<storm::PkgFiles>) },
			{ Name(L"lang"), L"PkgReader", Name(L"lang.PkgReader"), L"__ctor", list(3, Name(L"Type"), Name(L"lang.PkgFiles"), Name(L"Package")), address(&create3<storm::PkgReader, storm::PkgFiles *, storm::Package *>) },
			{ Name(L"lang"), L"FilesReader", Name(L"lang.FilesReader"), L"__ctor", list(3, Name(L"Type"), Name(L"lang.PkgFiles"), Name(L"Package")), address(&create3<storm::FilesReader, storm::PkgFiles *, storm::Package *>) },
			{ Name(L"core.lang"), L"SObject", Name(L"core.lang.SObject"), L"__ctor", list(1, Name(L"Type")), address(&create1<storm::SObject>) },
			{ Name(L"core.lang"), L"SStr", Name(L"core.lang.SStr"), L"__ctor", list(2, Name(L"Type"), Name(L"core.Str")), address(&create2<storm::SStr, storm::Str *>) },
			{ Name(L"core.lang"), L"SStr", Name(L"core.lang.SStr"), L"__ctor", list(2, Name(L"Type"), Name(L"core.lang.SStr")), address(&create2<storm::SStr, storm::SStr *>) },
			{ Name(L"core.lang"), L"SStr", Name(L"core.Bool"), L"equals", list(1, Name(L"core.Object")), address<Bool(CODECALL storm::SStr::*)(storm::Object *)>(&storm::SStr::equals) },
			{ Name(L""), L"VTest", Name(L"VTest"), L"__ctor", list(1, Name(L"Type")), address(&create1<storm::VTest>) },
			{ Name(L""), L"VTest", Name(L"core.Int"), L"returnOne", list(0), address<Int(CODECALL storm::VTest::*)()>(&storm::VTest::returnOne) },
			{ Name(L""), L"VTest", Name(L"core.Int"), L"returnTwo", list(0), address<Int(CODECALL storm::VTest::*)()>(&storm::VTest::returnTwo) },
			// END LIST
			{ Name(), null, Name(), L"", list(0), null },
		};
		return fns;
	}

}