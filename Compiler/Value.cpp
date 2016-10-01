#include "stdafx.h"
#include "Value.h"
#include "Type.h"
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

	Value thisPtr(Type *t) {
		if ((t->typeFlags & typeValue) == typeValue)
			return Value(t, false);
		else
			return Value(t, true);
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

	code::ValType Value::valType() const {
		return code::ValType(size(), isFloat());
	}

	BasicTypeInfo Value::typeInfo() const {
		BasicTypeInfo::Kind kind = BasicTypeInfo::nothing;

		if (!type) {
			kind = BasicTypeInfo::nothing;
		} else if (isBuiltIn()) {
			kind = type->builtInType();
		} else if (isValue()) {
			kind = BasicTypeInfo::user;
		} else {
			kind = BasicTypeInfo::ptr;
		}

		BasicTypeInfo r = {
			size().current(),
			kind,
		};
		return r;
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
		return type->builtInType() != BasicTypeInfo::user;
	}

	Bool Value::isFloat() const {
		if (!type)
			return false;
		if (ref)
			return false;
		return type->builtInType() != BasicTypeInfo::floatNr;
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
		if (isBuiltIn())
			return false;
		return type->isA(Object::stormType(type->engine));
	}

	Bool Value::isActor() const {
		if (!type)
			return false;
		if (isBuiltIn())
			return false;
		return type->isA(TObject::stormType(type->engine));
	}

	Str *toS(EnginePtr e, Value v) {
		Str *name = v.type ? v.type->name : new (e.v) Str(L"void");
		if (v.type != null && v.ref)
			name = *name + new (e.v) Str(L"&");
		return name;
	}

	StrBuf &operator <<(StrBuf &to, Value v) {
		return to << toS(to.engine(), v);
	}

	wostream &operator <<(wostream &to, const Value &v) {
		if (v.type) {
			to << v.type->name;
			if (v.ref)
				to << L"&";
		} else {
			to << L"void";
		}
		return to;
	}

}
