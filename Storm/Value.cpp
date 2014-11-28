#include "stdafx.h"
#include "Value.h"
#include "Type.h"
#include "Scope.h"
#include "Function.h"

namespace storm {

	Value::Value() : type(null) {}

	Value::Value(Type *t) : type(t) {}

	nat Value::size() const {
		if (type->flags & typeClass) {
			return 0; // by pointer
		} else {
			return type->size();
		}
	}

	bool Value::isBuiltIn() const {
		if (type->flags & typeClass) {
			return true; // by pointer
		} else {
			return type->isBuiltIn();
		}
	}

	code::Ref Value::destructor() const {
		if (type->flags & typeClass) {
			return code::Ref(type->engine.release);
		} else {
			TODO(L"Implement!");
			assert(false);
		}
	}

	bool Value::canStore(Type *x) const {
		return x->isA(type);
	}

	bool Value::canStore(const Value &v) const {
		return canStore(v.type);
	}

	void Value::output(wostream &to) const {
		if (type == null)
			to << L"void";
		else
			to << type->name;
	}

	bool Value::operator ==(const Value &o) const {
		return type == o.type;
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

	Value fnResultType(Auto<Scope> scope, const Name &fn, const vector<Value> &params) {
		Named *f = scope->find(fn);
		if (f == null) {
			return Value();
		} else if (Type *t = as<Type>(f)) {
			// We do not correctly handle this yet.
			assert(t->flags & typeClass);
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
