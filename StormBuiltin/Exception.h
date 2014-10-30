#pragma once
#include "Utils/Path.h"
#include "Utils/Exception.h"

class Error : public Exception {
public:
	Error(const String &msg, const Path &file = Path()) : msg(msg), file(file) {}
	virtual String what() const {
		if (file.isEmpty())
			return msg;
		else
			return ::toS(file) + L": " + msg;
	}
private:
	Path file;
	String msg;
};


