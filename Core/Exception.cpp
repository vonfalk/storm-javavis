#include "stdafx.h"
#include "Exception.h"
#include "StrBuf.h"

namespace storm {

	NException::NException() : stackTrace(engine()) {}

	NException::NException(const NException &o) : stackTrace(o.stackTrace) {}

	void NException::deepCopy(CloneEnv *env) {}

	Str *NException::message() const {
		StrBuf *b = new (this) StrBuf();
		message(b);
		return b->toS();
	}

	void NException::toS(StrBuf *to) const {
		message(to);
		if (stackTrace.any()) {
			*to << S("\n");
			stackTrace.format(to);
		}
	}

	NotSupported::NotSupported(const wchar *msg) {
		this->msg = new (this) Str(msg);
	}

	NotSupported::NotSupported(Str *msg) {
		this->msg = msg;
	}

	void NotSupported::message(StrBuf *to) const {
		*to << S("Operation not supported: ") << msg;
	}

	InternalError::InternalError(const wchar *msg) {
		this->msg = new (this) Str(msg);
	}

	InternalError::InternalError(Str *msg) {
		this->msg = msg;
	}

	void InternalError::message(StrBuf *to) const {
		*to << S("Internal error: ") << msg;
	}

	RuntimeError::RuntimeError(const wchar *msg) {
		this->msg = new (this) Str(msg);
	}

	RuntimeError::RuntimeError(Str *msg) {
		this->msg = msg;
	}

	void RuntimeError::message(StrBuf *to) const {
		*to << msg;
	}

	AbstractFnCalled::AbstractFnCalled(const wchar *name) : RuntimeError(name) {}

	AbstractFnCalled::AbstractFnCalled(Str *name) : RuntimeError(name) {}

	void AbstractFnCalled::message(StrBuf *to) const {
		*to << S("Abstract function called: ");
		RuntimeError::message(to);
	}

	StrError::StrError(const wchar *msg) {
		this->msg = new (this) Str(msg);
	}

	StrError::StrError(Str *msg) {
		this->msg = msg;
	}

	void StrError::message(StrBuf *to) const {
		*to << msg;
	}

	MapError::MapError(const wchar *msg) {
		this->msg = new (this) Str(msg);
	}

	MapError::MapError(Str *msg) {
		this->msg = msg;
	}

	void MapError::message(StrBuf *to) const {
		*to << S("Map error: ") << msg;
	}

	SetError::SetError(const wchar *msg) {
		this->msg = new (this) Str(msg);
	}

	SetError::SetError(Str *msg) {
		this->msg = msg;
	}

	void SetError::message(StrBuf *to) const {
		*to << S("Set error: ") << msg;
	}

	ArrayError::ArrayError(Nat id, Nat count) : id(id), count(count), msg(null) {}

	ArrayError::ArrayError(Nat id, Nat count, Str *msg) : id(id), count(count), msg(msg) {}

	void ArrayError::message(StrBuf *to) const {
		*to << S("Array error: Index ") << id << S(" out of bounds (of ") << count << S(").");
		if (msg)
			*to << S(" During ") << msg << S(".");
	}

}
