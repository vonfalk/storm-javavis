#pragma once
#include "SrcPos.h"
#include "Utils/Exception.h"

class Error : public Exception {
public:
	Error(const String &msg, const SrcPos &pos) : msg(msg), pos(pos) {}
	virtual String what() const {
		return ::toS(pos) + L": " + msg;
	}
private:
	SrcPos pos;
	String msg;
};


