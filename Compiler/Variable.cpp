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
		*to << type << L" ";
		Named::toS(to);
	}


	/**
	 * Member variable.
	 */

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


	/**
	 * Global variable.
	 */

	GlobalVar::GlobalVar(Str *name, Value type, NamedThread *thread) : Variable(name, type), owner(thread) {}

}
