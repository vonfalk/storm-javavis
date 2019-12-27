#include "stdafx.h"
#include "SQL.h"
#include "Core/Exception.h"

namespace sql {
	/////////////////////////////////////
	//			   Statement		  //
	/////////////////////////////////////

	Statement::Statement() {
		errorMsg = new(this) Str(L"");
	}

	Str* Statement::errMsg() const {
		return errorMsg;
	}

	Statement::Iter::Iter() : owner(null) {}

	Statement::Iter::Iter(const Statement *stmt) : owner(stmt) {}

	Row * Statement::Iter::next() {
		return owner->fetch();
	}

	Statement::Iter Statement::iter() {
		return Iter(this);
	}

	/////////////////////////////////////
	//			   Database			   //
	/////////////////////////////////////

	EmptyDB::EmptyDB() {}

	Statement * EmptyDB::prepare(Str *str) {
		throw NotSupported (L"Empty Database");
	}

	Long EmptyDB::lastRowId() const {
		throw NotSupported (L"Empty Database");
	}

	void EmptyDB::close() {
		throw NotSupported (L"Empty Database");
	}

	Array<Str*>* EmptyDB::tables() {
		throw NotSupported (L"Empty Database");
	}

	Schema * STORM_FN EmptyDB::schema(Str * str) {
		throw NotSupported (L"Empty Database");
	}

	/////////////////////////////////////
	//				 Row			   //
	/////////////////////////////////////

	Row::Row() : v(null) {}

	Row::Row(Array<Variant> * v) : v(v) {}

	Str* Row::getStr(Int idx) {
		if (v)
			return (Str*)(v -> at(idx).getObject());

		return new(this) Str();
	}

	Int* Row::getInt(int idx) {
		if (v)
			return (Int*)(v -> at(idx).getValue());

		return new(this) Int();
	}

	Double* Row::getDouble(int idx) {
		if (v)
			return (Double*)(v -> at(idx).getValue());

		return new(this) Double();
	}

}
