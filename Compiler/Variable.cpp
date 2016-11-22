#include "stdafx.h"
#include "Variable.h"
#include "Type.h"
#include "Core/Str.h"
#include "Core/StrBuf.h"

namespace storm {

	Variable::Variable(Str *name, Value type) :
		Named(name), type(type) {}

	Variable::Variable(Str *name, Value type, Type *member) :
		Named(name, new (name) Array<Value>(1, Value(member))), type(type) {}


	void Variable::toS(StrBuf *to) const {
		*to << type << L" " << identifier();
	}


	MemberVar::MemberVar(Str *name, Value type, Type *member) : Variable(name, type, member) {
		hasLayout = false;
	}

	Offset MemberVar::offset() const {
		if (!hasLayout) {
			owner()->doLayout();
		}
		return off;
	}

	void MemberVar::setOffset(Offset to) {
		hasLayout = true;
		off = to;
	}

	Type *MemberVar::owner() const {
		assert(!params->empty());
		return params->at(0).type;
	}

}