#include "stdafx.h"
#include "Exception.h"
#include "Type.h"
#include "Core/Str.h"

namespace storm {

	InternalTypeError::InternalTypeError(const String &context, const Type *expected, const Type *got) :
		InternalError(context + L": expected " +
					::toS(expected->identifier()) + L", got " +
					(got ? ::toS(got->identifier()) : String(L"null"))) {}


	void CodeError::output(wostream &to) const {
		to << what();
	}


}
