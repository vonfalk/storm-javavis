#include "stdafx.h"
#include "Value.h"
#include "Type.h"
#include "Scope.h"
#include "Function.h"
#include "Exception.h"
#include "Engine.h"
#include "Lib/Handle.h"

namespace storm {

	Str *toS(EnginePtr e, const Value &v) {
		return CREATE(Str, e.v, ::toS(v));
	}

	bool isClass(Type *t) {
		return (t->flags & typeClass) != 0;
	}

	Value Value::thisPtr(Type *t) {
		bool ref = false;
		if (t->flags & typeValue) {
			ref = true;
		} else if (t->flags & typeClass) {
			ref = false;
		} else {
			assert(false, "You do want to set either typeValue or typeClass on your types!");
		}
		return Value(t, (t->flags & typeValue) != 0);
	}

	Value::Value() : type(null), ref(false) {}

	Value::Value(Type *t, bool ref) : type(t), ref(ref) {}

	Value::Value(Par<Type> t) : type(t.borrow()), ref(false) {}

	Value::Value(Par<Type> t, Bool ref) : type(t.borrow()), ref(ref) {}

	Type *Value::getType() const {
		type->addRef();
		return type;
	}

	Bool Value::isRef() const {
		return ref;
	}

	const Handle &Value::handle() const {
		if (type) {
			assert(!ref, "Handles to references is not yet supported!");
			return type->handle();
		} else {
			return storm::handle<void>();
		}
	}

	void Value::deepCopy(Par<CloneEnv> env) {
		// Named objects are threaded, it is OK to not do anything here.
	}

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

	bool Value::returnInReg() const {
		if (ref)
			return true;
		if (isBuiltIn())
			return true;
		return !isValue();
	}

	bool Value::isBuiltIn() const {
		if (type == null)
			return true;
		return type->builtInType() != BasicTypeInfo::user;
	}

	bool Value::isValue() const {
		if (type == null)
			return false;
		if (isBuiltIn())
			return false;
		return (type->flags & typeValue) == typeValue;
	}

	bool Value::isClass() const {
		if (type == null)
			return false;
		if (isBuiltIn())
			return false;
		return (type->flags & typeClass) == typeClass;
	}

	code::Value Value::destructor() const {
		if (ref) {
			return code::Value();
		} else if (type) {
			if (type->flags & typeClass)
				return type->engine.fnRefs.release;
			else if (Function *dtor = type->destructor())
				return dtor->ref();
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
				return ctor->ref();
			else
				return code::Value();
		} else {
			return code::Value();
		}
	}

	code::Value Value::defaultCtor() const {
		if (ref)
			return code::Value();

		if (Function *ctor = type->defaultCtor())
			return ctor->ref();

		return code::Value();
	}

	code::Value Value::assignFn() const {
		if (ref)
			return code::Value();

		if (!isValue())
			return code::Value();

		if (Function *ctor = type->assignFn())
			return ctor->ref();

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
			if (isClass() && v.isClass())
				return false;
		return canStore(v.type);
	}

	bool Value::matches(const Value &v, MatchFlags flags) const {
		bool r = canStore(v);
		if (flags & matchNoInheritance)
			r &= type == v.type;
		return r;
	}

	void Value::mustStore(const Value &v, const SrcPos &p) const {
		if (!canStore(v))
			throw TypeError(p, *this, v);
	}

	BasicTypeInfo Value::typeInfo() const {
		BasicTypeInfo::Kind kind = BasicTypeInfo::nothing;

		if (isClass()) {
			kind = BasicTypeInfo::ptr;
		} else if (isValue()) {
			kind = BasicTypeInfo::user;
		} else if (type != null) {
			kind = type->builtInType();
		}

		BasicTypeInfo r = {
			size().current(),
			kind,
		};
		return r;
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


	Value fnResultType(const Scope &scope, Par<Name> fn) {
		Named *f = scope.find(fn);
		if (f == null) {
			return Value();
		} else if (Type *t = as<Type>(f)) {
			// We do not correctly handle this yet.
			assert(t->flags & typeClass);
			return Value(t);
		} else if (Function *fn = as<Function>(f)) {
			return fn->result;
		} else {
			assert(false); // Unhandled type!
			return Value();
		}
	}

#ifdef VS
	ValList valList(nat count, ...) {
		vector<Value> r;
		r.reserve(count);

		va_list l;
		va_start(l, count);

		for (nat i = 0; i < count; i++) {
			r.push_back(va_arg(l, Value));
		}

		va_end(l);
		return r;
	}
#endif
}
