#include "stdafx.h"
#include "Value.h"
#include "Type.h"
#include "Engine.h"
#include "Core/Str.h"
#include "Core/StrBuf.h"
#include "Utils/TypeInfo.h"

namespace storm {

	Value::Value() : type(null), ref(false) {}

	Value::Value(Type *t) : type(t), ref(false) {}

	Value::Value(Type *t, Bool ref) : type(t), ref(ref) {}

	void Value::deepCopy(CloneEnv *env) {}

	Value Value::asRef() const {
		return Value(type, true);
	}

	Value Value::asRef(Bool ref) const {
		return Value(type, ref);
	}

	Bool Value::operator ==(Value o) const {
		// Treat null and null& the same.
		if (type == null && o.type == null)
			return true;
		return type == o.type
			&& ref == o.ref;
	}

	Bool Value::operator !=(Value o) const {
		return !(*this == o);
	}

	Bool Value::canStore(Type *o) const {
		if (type == null)
			return true; // void can 'store' all types.
		if (o == null)
			return false; // void can not be 'upcasted' to anything else.
		return o->isA(type);
	}

	Bool Value::canStore(Value v) const {
		// For objects: We can not create references from values.
		// For values, we need to be able to. In the future, maybe const refs from values?
		if (ref && !v.ref)
			if (isClass() || v.isClass())
				return false;
		return canStore(v.type);
	}

	Bool Value::matches(Value v, NamedFlags flags) const {
		if (flags & namedMatchNoInheritance)
			return type == v.type;
		else
			return canStore(v);
	}

	Value common(Value a, Value b) {
		if (a.type == null || b.type == null)
			return Value();

		for (Type *t = a.type; t; t = t->super()) {
			if (b.type->isA(t))
				return Value(t);
		}
		return Value();
	}

	Value thisPtr(Type *t) {
		if ((t->typeFlags & typeValue) == typeValue)
			return Value(t, true);
		else
			return Value(t, false);
	}

	Size Value::size() const {
		if (!type) {
			return Size();
		} else if (ref) {
			return Size::sPtr;
		} else if (isValue() || isBuiltIn()) {
			return type->size();
		} else {
			return Size::sPtr;
		}
	}

	static TypeKind::T convert(code::primitive::Kind k) {
		switch (k) {
		case code::primitive::none:
			return TypeKind::nothing;
		case code::primitive::pointer:
			return TypeKind::ptr;
		case code::primitive::integer:
			return TypeKind::signedNr;
		case code::primitive::real:
			return TypeKind::floatNr;
		}
		return TypeKind::nothing;
	}

	BasicTypeInfo Value::typeInfo() const {
		BasicTypeInfo::Kind kind = TypeKind::nothing;

		if (!type) {
			kind = TypeKind::nothing;
		} else if (ref) {
			kind = TypeKind::ptr;
		} else {
			code::TypeDesc *desc = type->typeDesc();
			if (code::PrimitiveDesc *p = as<code::PrimitiveDesc>(desc)) {
				kind = convert(p->v.kind());
			} else if (as<code::SimpleDesc>(desc)) {
				kind = TypeKind::userTrivial;
			} else if (as<code::ComplexDesc>(desc)) {
				kind = TypeKind::userComplex;
			} else {
				assert(false);
			}
		}

		BasicTypeInfo r = {
			size().current(),
			kind,
		};
		return r;
	}

	code::TypeDesc *Value::desc(Engine &e) const {
		if (!type)
			return e.voidDesc();
		return type->typeDesc();
	}

	code::TypeDesc *desc(EnginePtr e, Value v) {
		return v.desc(e.v);
	}

	Bool Value::returnInReg() const {
		if (ref)
			return true;
		if (isBuiltIn())
			return true;
		return !isValue();
	}

	Bool Value::isBuiltIn() const {
		if (!type)
			// Void is considered built-in.
			return true;

		// Class types are never built in types, and we can not distinguish them from pointers.
		if (type->typeFlags & typeClass)
			return false;

		code::TypeDesc *desc = type->typeDesc();
		if (as<code::PrimitiveDesc>(desc)) {
			return true;
		} else {
			return false;
		}
	}

	Bool Value::isValue() const {
		if (!type)
			return false;
		if (isBuiltIn())
			return false;
		return (type->typeFlags & typeValue) == typeValue;
	}

	Bool Value::isClass() const {
		if (!type)
			return false;
		if (type->typeFlags & typeClass)
			return type->isA(Object::stormType(type->engine));
		else
			return false;
	}

	Bool Value::isActor() const {
		if (!type)
			return false;
		if (type->typeFlags & typeClass)
			return type->isA(TObject::stormType(type->engine));
		else
			return false;
	}

	Bool Value::isHeapObj() const {
		if (!type)
			return false;
		return (type->typeFlags & typeClass) == typeClass;
	}

	Bool Value::isPtr() const {
		if (!type)
			return false;
		if ((type->typeFlags & typeClass) == typeClass)
			return true;
		return false;
	}

	code::Operand Value::copyCtor() const {
		if (isValue()) {
			Function *ctor = type->copyCtor();
			if (!ctor) {
				WARNING(L"Not returning a proper copy-constructor!");
				return code::Operand();
			}

			return code::Ref(ctor->ref());
		} else {
			return code::Operand();
		}
	}

	code::Operand Value::destructor() const {
		if (isValue()) {
			if (Function *dtor = type->destructor())
				return dtor->ref();
		}

		return code::Operand();
	}

	StrBuf &operator <<(StrBuf &to, Value v) {
		if (v.type) {
			to << v.type->identifier();
			if (v.ref)
				to << L"&";
		} else {
			to << L"void";
		}
		return to;
	}

	wostream &operator <<(wostream &to, const Value &v) {
		if (v.type) {
			to << v.type->identifier()->c_str();
			if (v.ref)
				to << L"&";
		} else {
			to << L"void";
		}
		return to;
	}

#ifdef VISUAL_STUDIO
	Array<Value> *valList(Engine &e, nat count, ...) {
		Array<Value> *r = new (e) Array<Value>();
		r->reserve(count);

		va_list l;
		va_start(l, count);

		for (nat i = 0; i < count; i++) {
			r->push(va_arg(l, Value));
		}

		va_end(l);
		return r;
	}
#endif

}
