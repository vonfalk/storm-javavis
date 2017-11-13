#pragma once

#include "StackTrace.h"
#include "Platform.h"

class EXCEPTION_EXPORT Exception : public Printable {
public:
	Exception();
	virtual ~Exception();

	virtual String what() const = 0;

	// Stack trace
	StackTrace stackTrace;

protected:
	virtual void output(wostream &to) const;
};

// Generic error supposed to be reported to the user, not falling under any other cathegory.
class EXCEPTION_EXPORT UserError : public Exception {
public:
	UserError(const String &msg) : msg(msg) {}

	virtual String what() const { return msg; }
private:
	String msg;
};

class EXCEPTION_EXPORT NoSuchFile : public Exception {
public:
	NoSuchFile(const String &file) : file(file) {};
	NoSuchFile(const NoSuchFile &other) : file(other.file) {};
	NoSuchFile &operator=(const NoSuchFile &other) { file = other.file; return *this; };

	virtual String what() const { return L"The file " + file + L" does not exist."; };
protected:
	String file;
};

class EXCEPTION_EXPORT CannotOpen : public Exception {
public:
	CannotOpen(const String &path) : path(path) {};
	CannotOpen(const CannotOpen &other) : path(other.path) {};
	CannotOpen &operator =(const CannotOpen &other) { path = other.path; return *this; };

	virtual String what() const { return L"Cannot open the file " + path; };
private:
	String path;
};

class EXCEPTION_EXPORT LoadError : public Exception {
public:
	LoadError(const String &error) : error(error) {};
	LoadError(const LoadError &other) : error(other.error) {};
	LoadError &operator=(const LoadError &other) { error = other.error; return *this; };

	virtual String what() const { return error; };
private:
	String error;
};

class EXCEPTION_EXPORT SaveError : public Exception {
public:
	SaveError(const String &error) : error(error) {};
	SaveError(const SaveError &other) : error(other.error) {};
	SaveError &operator=(const SaveError &other) { error = other.error; return *this; };

	virtual String what() const { return error; };
private:
	String error;
};
