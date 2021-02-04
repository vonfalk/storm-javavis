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
	//				 Row			   //
	/////////////////////////////////////

	Row::Row() : v(null) {}

	Row::Row(Array<Variant> * v) : v(v) {}

	Str* Row::getStr(Nat idx) {
		if (v)
			return v->at(idx).get<Str *>();
		else
			throw new (this) ArrayError(0, 0);
	}

	Int Row::getInt(Nat idx) {
		if (v)
			return (Int)v->at(idx).get<Long>();
		else
			throw new (this) ArrayError(0, 0);
	}

	Long Row::getLong(Nat idx) {
		if (v)
			return v->at(idx).get<Long>();
		else
			throw new (this) ArrayError(0, 0);
	}

	Double Row::getDouble(Nat idx) {
		if (v)
			return v->at(idx).get<Double>();
		else
			throw new (this) ArrayError(0, 0);
	}

}
