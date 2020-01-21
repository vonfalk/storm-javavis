#pragma once
#include "Var.h"
#include "Block.h"
#include "Core/Exception.h"

namespace code {

	// Thrown when an instruction contains invalid data for some reason.
	class EXCEPTION_EXPORT InvalidValue : public storm::NException {
		STORM_CLASS;
	public:
		InvalidValue(const wchar *what) {
			error = new (engine()) Str(what);
		}
		STORM_CTOR InvalidValue(Str *what) {
			error = what;
		}
		virtual void STORM_FN message(StrBuf *to) const {
			*to << S("Invalid value: ") << error;
		}

	private:
		Str *error;
	};

	class EXCEPTION_EXPORT BlockBeginError : public storm::NException {
		STORM_CLASS;
	public:
		STORM_CTOR BlockBeginErrror() {
			msg = new (engine()) Str(S("The parent scope must be entered before a child scope."));
		}
		STORM_CTOR BlockBeginError(Str *msg) {
			this->msg = msg;
		}
		virtual void STORM_FN message(StrBuf *to) const {
			*to << msg;
		}
	private:
		Str *msg;
	};

	class EXCEPTION_EXPORT BlockEndError : public storm::NException {
		STORM_CLASS;
	public:
		STORM_CTOR BlockEndError() {
			msg = new (engine()) Str(S("The scope is not the topmost active scope."));
		}
		BlockEndError(const String &msg) {
			this->msg = msg;
		}
		virtual void STORM_FN message(StrBuf *to) const {
			*to << msg;
		}
	private:
		Str *msg;
	};

	class EXCEPTION_EXPORT FrameError : public storm::NException {
		STORM_CLASS;
	public:
		STORM_CTOR FrameError() {}
		virtual void STORM_FN message(StrBuf *to) const {
			*to << S("Trying to use an invalid frame, part or variable.");
		}
	};

	class EXCEPTION_EXPORT DuplicateLabelError : public storm::NException {
		STORM_CLASS;
	public:
		STORM_CTOR DuplicateLabelError(Nat id) {
			this->id = id;
		}

		Nat id;

		virtual void STORM_FN message(StrBuf *to) const {
			*to << S("Duplicate usage of label ") << id << S(".");
		}
	};

	class EXCEPTION_EXPORT UnusedLabelError : public storm::NException {
		STORM_CLASS;
	public:
		STORM_CTOR UnusedLabelError(Nat id) {
			this->id = id;
		}

		Nat id;

		virtual void STORM_FN message(StrBuf *to) const {
			*to << S("Use of undefined label ") << id << S(".");
		}
	};

	class EXCEPTION_EXPORT VariableUseError : public storm::NException {
		STORM_CLASS;
	public:
		STORM_CTOR VariableUseError(Var v, Part p) {
			var = v;
			part = p;
		}

		Var var;
		Part part;

		virtual void STORM_FN message(StrBuf *to) const {
			*to << S("Trying to use ") << var << S(" in ") << part << S(", where it is not accessible.");
		}
	};
}

