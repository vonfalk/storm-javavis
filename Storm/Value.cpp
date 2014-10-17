#include "stdafx.h"
#include "Value.h"
#include "Type.h"

namespace storm {

	Value::Value() : type(null) {}

	Value::Value(Type *t) : type(t) {}

	void Value::output(wostream &to) const {
		if (type == null)
			to << L"null";
		else
			to << type->name;
	}

	bool Value::operator ==(const Value &o) const {
		return type == o.type;
	}

	bool Value::operator <(const Value &o) const {
		return type < o.type;
	}

}
