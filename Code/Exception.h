#pragma once

namespace code {

	// Thrown when an instruction contains invalid data for some reason.
	class EXCEPTION_EXPORT InvalidValue : public Exception {
		String error;
	public:
		InvalidValue(const String &what) : error(what) {}
		virtual String what() const { return error; }
	};

	class EXCEPTION_EXPORT BlockBeginError : public Exception {
	public:
		BlockBeginError() : msg(L"The parent scope must be entered before a child scope.") {}
		BlockBeginError(const String &msg) : msg(msg) {}
		String what() const { return msg; }
	private:
		String msg;
	};

	class EXCEPTION_EXPORT BlockEndError : public Exception {
	public:
		BlockEndError() : msg(L"The scope is not the topmost active scope.") {}
		BlockEndError(const String &msg) : msg(msg) {}
		String what() const { return msg; }
	private:
		String msg;
	};

	class EXCEPTION_EXPORT FrameError : public Exception {
	public:
		String what() const { return L"Trying to use an invalid frame, part or variable."; }
	};

	class EXCEPTION_EXPORT DuplicateLabelError : public Exception {
	public:
		DuplicateLabelError(nat id) : id(id) {}

		nat id;

		String what() const { return String(L"Duplicate usage of label ") + toS(id) + L"."; }
	};

	class EXCEPTION_EXPORT UnusedLabelError : public Exception {
	public:
		UnusedLabelError(nat id) : id(id) {}

		nat id;

		String what() const { return String(L"Use of undefined label ") + toS(id) + L"."; }
	};

	class EXCEPTION_EXPORT VariableUseError : public Exception {
	public:
		VariableUseError(Var v, Part p) : var(v), part(p) {}

		Var var;
		Part part;

		String what() const { return String(L"Trying to use " + toS(var) + L" in " + toS(part) + L", where it is not accessible."); }
	};
}

