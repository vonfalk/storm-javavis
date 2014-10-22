#include "stdafx.h"
#include "Value.h"
#include "Type.h"
#include "Scope.h"
#include "Function.h"

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

	/**
	 * Helpers
	 */

	static Value fnResultType(NameOverload *no) {
		if (Function *fn = as<Function>(no)) {
			return fn->result;
		} else {
			assert(false); // Unhandled type!
			return Value();
		}
	}

	Value fnResultType(Scope *scope, const Name &fn, const vector<Value> &params) {
		Named *f = scope->find(fn);
		if (f == null) {
			return Value();
		} else if (Type *t = as<Type>(f)) {
			// We do not correctly handle this yet.
			assert((t->flags & typeValue) != typeValue);
			return Value(t);
		} else if (Overload *o = as<Overload>(f)) {
			NameOverload *no = o->find(params);
			return fnResultType(no);
		} else {
			assert(false); // Unhandled type!
			return Value();
		}
	}

}
