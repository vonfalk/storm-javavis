#include "stdafx.h"
#include "Value.h"
#include "Type.h"
#include "Scope.h"
#include "Function.h"
#include "Exception.h"

namespace storm {

	Value::Value() : type(null), ref(false) {}

	Value::Value(Type *t, bool ref) : type(t), ref(ref) {}

	nat Value::size() const {
		if (ref) {
			return 0; // pointer-sized
		} else if (!type) {
			return 0;
		} else if (type->flags & typeClass) {
			return 0; // by pointer
		} else {
			return type->size();
		}
	}

	bool Value::isBuiltIn() const {
		if (!type || ref) {
			return true;
		} else if (type->flags & typeClass) {
			return true; // by pointer
		} else {
			return type->isBuiltIn();
		}
	}

	code::Value Value::destructor() const {
		if (ref) {
			return code::Value();
		} else if (type && (type->flags & typeClass)) {
			return code::Ref(type->engine.release);
		} else {
			return type->destructor();
		}
	}

	bool Value::refcounted() const {
		if (!type)
			return false;
		return (type->flags & typeClass) != 0;
	}

	bool Value::canStore(Type *x) const {
		return x->isA(type);
	}

	bool Value::canStore(const Value &v) const {
		return canStore(v.type);
	}

	void Value::mustStore(const Value &v, const SrcPos &p) const {
		if (!canStore(v))
			throw TypeError(p, L"Expected " + ::toS(this) + L", got " + ::toS(v));
	}

	void Value::output(wostream &to) const {
		if (type == null)
			to << L"void";
		else
			to << type->name;
		if (ref)
			to << "&";
	}

	bool Value::operator ==(const Value &o) const {
		return type == o.type && ref == o.ref;
	}

	Value common(const Value &a, const Value &b) {
		for (Type *t = a.type; t; t = t->super()) {
			if (b.type->isA(t))
				return Value(t);
		}
		return Value();
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

	Value fnResultType(const Scope &scope, const Name &fn, const vector<Value> &params) {
		Named *f = scope.find(fn);
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
