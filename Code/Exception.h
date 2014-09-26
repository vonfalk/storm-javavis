#pragma once
#include "Utils/Exception.h"

namespace code {

	class BlockBeginError : public Exception {
	public:
		String what() const { return L"The parent scope must be entered before a child scope."; }
	};

	class BlockEndError : public Exception {
	public:
		String what() const { return L"The scope is not the topmost active scope."; }
	};

	class FrameError : public Exception {
	public:
		String what() const { return L"Trying to use an invalid variable or block."; }
	};

	class DuplicateLabelError : public Exception {
	public:
		DuplicateLabelError(nat id) : id(id) {}

		nat id;

		String what() const { return String(L"Duplicate usage of label ") + toS(id) + L"."; }
	};

	class UnusedLabelError : public Exception {
	public:
		UnusedLabelError(nat id) : id(id) {}

		nat id;

		String what() const { return String(L"Use of undefined label ") + toS(id) + L"."; }
	};
}