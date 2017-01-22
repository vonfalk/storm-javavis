#include "stdafx.h"
#include "Exception.h"
#include "Type.h"

namespace storm {

	InternalTypeError::InternalTypeError(const String &context, const Type *expected, const Type *got) :
		InternalError(context + L": expected " +
					expected->identifier() + L", got " +
					(got ? got->identifier() : String(L"null"))) {}


	void CodeError::output(wostream &to) const {
		to << what();
	}

}
