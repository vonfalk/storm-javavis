#include "stdafx.h"
#include "Exception.h"
#include "Type.h"
#include "Core/Str.h"

namespace storm {

	Str *CodeError::messageText() const {
		StrBuf *buf = new (this) StrBuf();
		messageText(buf);
		return buf->toS();
	}

	InternalTypeError::InternalTypeError(const wchar *context, Type *expected, Type *got) :
		InternalError(TO_S(engine(), context << S(": expected ") << expected->identifier()
									<< S(", got ") << (got ? got->identifier() : new (engine()) Str(S("null"))))) {}

	InternalTypeError::InternalTypeError(Str *context, Type *expected, Type *got) :
		InternalError(TO_S(engine(), context << S(": expected ") << expected->identifier()
									<< S(", got ") << (got ? got->identifier() : new (engine()) Str(S("null"))))) {}


	SyntaxError::SyntaxError(SrcPos where, const wchar *msg) :
		CodeError(where), text(new (engine()) Str(msg)) {}

	SyntaxError::SyntaxError(SrcPos where, Str *msg) :
		CodeError(where), text(msg) {}

	TypeError::TypeError(SrcPos where, const wchar *msg) : CodeError(where), msg(new (engine()) Str(msg)) {}
	TypeError::TypeError(SrcPos where, Str *msg) : CodeError(where), msg(msg) {}
	TypeError::TypeError(SrcPos where, Value expected, ExprResult got)
		: CodeError(where),
		  msg(TO_S(engine(), S("Expected ") << expected << S(" but got ") << got)) {}
	TypeError::TypeError(SrcPos where, Value expected, Value got)
		: CodeError(where),
		  msg(TO_S(engine(), S("Expected ") << expected << S(" but got ") << got)) {}

	TypedefError::TypedefError(SrcPos pos, const wchar *msg) : CodeError(pos), msg(new (engine()) Str(msg)) {}
	TypedefError::TypedefError(SrcPos pos, Str *msg) : CodeError(pos), msg(msg) {}

	InstantiationError::InstantiationError(SrcPos pos, const wchar *msg) : CodeError(pos), msg(new (engine()) Str(msg)) {}
	InstantiationError::InstantiationError(SrcPos pos, Str *msg) : CodeError(pos), msg(msg) {}


}
