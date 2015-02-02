#include "stdafx.h"
#include "Value.h"
#include "Type.h"
#include "Scope.h"
#include "Function.h"
#include "Exception.h"

namespace storm {

	Value Value::thisPtr(Type *t) {
		bool ref = false;
		if (t->flags & typeValue) {
			ref = true;
		} else if (t->flags & typeClass) {
			ref = false;
		} else {
			assert(("You do want to set either typeValue or typeClass on your types!", false));
		}
		return Value(t, (t->flags & typeValue) != 0);
	}

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

	bool Value::returnOnStack() const {
		if (ref)
			return true;
		if (isBuiltIn())
			return true;
		return !isValue();
	}

	bool Value::isBuiltIn() const {
		if (type == null)
			return true;
		return type->isBuiltIn();
	}

	bool Value::isValue() const {
		if (type == null)
			return false;
		if (isBuiltIn())
			return false;
		return (type->flags & typeValue) == typeValue;
	}

	code::Value Value::destructor() const {
		if (ref) {
			return code::Value();
		} else if (type) {
			if (type->flags & typeClass)
				return code::Ref(type->engine.release);
			else if (Function *dtor = type->destructor())
				return code::Ref(dtor->ref());
			else
				return code::Value();
		} else {
			return code::Value();
		}
	}

	code::Value Value::copyCtor() const {
		if (ref) {
			return code::Value();
		} else if (type) {
			if (type->flags & typeClass)
				return code::Value();
			else if (Function *ctor = type->copyCtor())
				return code::Ref(ctor->ref());
			else
				return code::Value();
		} else {
			return code::Value();
		}
	}

	code::Value Value::defaultCtor() const {
		if (ref)
			return code::Value();

		if (!isValue())
			return code::Value();

		if (Function *ctor = type->defaultCtor())
			return code::Ref(ctor->ref());

		return code::Value();
	}

	bool Value::refcounted() const {
		if (!type)
			return false;
		return (type->flags & typeClass) != 0;
	}

	Value Value::asRef(bool z) const {
		Value v(*this);
		v.ref = z;
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
		// For objects: we can not create references from values.
		// For values, we need to be able to. In the future, maybe const refs from values?
		if (ref && !v.ref)
			if (!isValue() && !v.isValue())
				return false;
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
			to << type->identifier();
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
