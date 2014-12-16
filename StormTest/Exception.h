#pragma once
#include "Utils/Exception.h"

class TestError : public Exception {
public:
	TestError(const String &msg) : msg(msg) {}
	virtual String what() const { return msg; }
private:
	String msg;
};
