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
		if (v) {
			Variant z = v->at(idx);
			if (z.empty())
				return new (this) Str();
			else
				return z.get<Str *>();
		} else {
			throw new (this) ArrayError(0, 0);
		}
	}

	Int Row::getInt(Nat idx) {
		if (v) {
			Variant z = v->at(idx);
			if (z.empty())
				return 0;
			else
				return (Int)z.get<Long>();
		} else {
			throw new (this) ArrayError(0, 0);
		}
	}

	Long Row::getLong(Nat idx) {
		if (v) {
			Variant z = v->at(idx);
			if (z.empty())
				return 0;
			else
				return z.get<Long>();
		} else {
			throw new (this) ArrayError(0, 0);
		}
	}

	Double Row::getDouble(Nat idx) {
		if (v) {
			Variant z = v->at(idx);
			if (z.empty())
				return 0;
			else
				return (Int)z.get<Double>();
		} else {
			throw new (this) ArrayError(0, 0);
		}
	}

	Bool Row::isNull(Nat idx) {
		if (v) {
			return v->at(idx).empty();
		} else {
			throw new (this) ArrayError(0, 0);
		}
	}

}
