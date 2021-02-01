#include "stdafx.h"
#include "SQL.h"
#include "Core/Exception.h"

namespace sql {
	/////////////////////////////////////
	//			   Statement		  //
	/////////////////////////////////////

	Statement::Statement() {}

	Statement::Iter::Iter(Statement *stmt) : owner(stmt) {}

	Row *Statement::Iter::next() {
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
		throw new (this) NotSupported(S("Empty Database"));
	}

	void EmptyDB::close() {
		throw new (this) NotSupported(S("Empty Database"));
	}

	Array<Str*>* EmptyDB::tables() {
		throw new (this) NotSupported(S("Empty Database"));
	}

	Schema * STORM_FN EmptyDB::schema(Str * str) {
		throw new (this) NotSupported(S("Empty Database"));
	}

	/////////////////////////////////////
	//				 Row			   //
	/////////////////////////////////////

	Row::Row() : v(null) {}

	Row::Row(Array<Variant> * v) : v(v) {}

	Str* Row::getStr(Nat idx) {
		if (v)
			return v->at(idx).get<Str *>();
		else
			return new (this) Str();
	}

	Int Row::getInt(Nat idx) {
		if (v)
			return v->at(idx).get<Int>();
		else
			return 0;
	}

	Double Row::getDouble(Nat idx) {
		if (v)
			return v->at(idx).get<Double>();
		else
			return 0;
	}

}
