#include "stdafx.h"
#include "Value.h"
#include "Type.h"
#include "Core/Str.h"
#include "Core/StrBuf.h"

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
		if (t->value())
			return Value(t, false);
		else
			return Value(t, true);
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

	wostream &operator <<(wostream &to, Value v) {
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
