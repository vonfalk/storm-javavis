#include "stdafx.h"
#include "Value.h"
#include "Type.h"
#include "Scope.h"
#include "Function.h"
#include "Exception.h"

namespace storm {

	Value::Value() : type(null), ref(false) {}

	Value::Value(Type *t, bool ref) : type(t), ref(ref) {}

	Size Value::size() const {
		if (ref) {
			// References are passed by pointer.
			return Size::sPtr;
		} else if (!type) {
			// null
			return Size();
		} else if (type->flags & typeClass) {
			// by pointer
			return Size::sPtr;
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
		} else if (type) {
			return type->destructor();
		} else {
			return code::Value();
		}
	}

	bool Value::refcounted() const {
		if (!type)
			return false;
		return (type->flags & typeClass) != 0;
	}

	Value Value::asRef() const {
		Value v(*this);
		v.ref = true;
		return v;
	}

	bool Value::canStore(Type *x) const {
		if (type == null)
			return true; // void can 'store' all types.
		if (x == null)
			return false; // void can not be 'upcasted' to anything else.
		return x->isA(type);
	}

	bool Value::canStore(const Value &v) const {
		if (ref && !v.ref)
			return false; // Can not create references from a value.
		return canStore(v.type);
	}

	void Value::mustStore(const Value &v, const SrcPos &p) const {
		if (!canStore(v))
			throw TypeError(p, *this, v);
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
