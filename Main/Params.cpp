#include "stdafx.h"
#include "Params.h"

struct StatePtr;
typedef StatePtr (*State)(const wchar *, Params &);

struct StatePtr {
	StatePtr() : p(null) {}
    StatePtr(State p) : p(p) {}
	StatePtr operator () (const wchar *s, Params &l) { return (*p)(s, l); }
	operator bool() { return p != null; }
    State p;
};

#define PARSE_ERROR(msg)						\
	result.mode = Params::modeError;			\
	result.modeParam = msg;						\
	result.modeParam2 = null;					\
	return StatePtr()

#define EXPECT_MORE(msg)						\
	if (arg == null) {							\
		PARSE_ERROR(msg);						\
	}

static StatePtr start(const wchar *arg, Params &result);

static StatePtr function(const wchar *arg, Params &result) {
	EXPECT_MORE(L"Missing function name.");
	result.modeParam = arg;
	return &start;
}

static StatePtr replCommand(const wchar *arg, Params &result) {
	EXPECT_MORE(L"Missing repl command.");
	result.modeParam2 = arg;
	return &start;
}

static StatePtr root(const wchar *arg, Params &result) {
	EXPECT_MORE(L"Missing root path.");
	if (result.root) {
		PARSE_ERROR(L"Root already set once!");
	}
	result.root = arg;
	return &start;
}

static StatePtr importPath(const wchar *arg, Params &result) {
	EXPECT_MORE(L"Missing import path.");
	result.import.back().path = arg;
	return &start;
}

static StatePtr import(const wchar *arg, Params &result) {
	EXPECT_MORE(L"Missing import target.");
	Import i = {
		null,
		arg
	};
	result.import.push_back(i);
	return &importPath;
}

static StatePtr start(const wchar *arg, Params &result) {
	if (arg == null) {
		return StatePtr();
	} else if (wcscmp(arg, L"-?") == 0 || wcscmp(arg, L"--help") == 0) {
		result.mode = Params::modeHelp;
		return StatePtr();
	} else if (wcscmp(arg, L"-f") == 0) {
		result.mode = Params::modeFunction;
		return &function;
	} else if (wcscmp(arg, L"-c") == 0) {
		result.mode = Params::modeRepl;
		return &replCommand;
	} else if (wcscmp(arg, L"-i") == 0) {
		return &import;
	} else if (wcscmp(arg, L"-r") == 0) {
		return &root;
	} else if (wcscmp(arg, L"--server") == 0) {
		result.mode = Params::modeServer;
		return StatePtr();
	} else {
		result.mode = Params::modeRepl;
		result.modeParam = arg;
		return &start;
	}
}

Params::Params(int argc, const wchar *argv[])
	: mode(modeRepl),
	  root(null),
	  modeParam(L"bs"),
	  modeParam2(null),
	  import() {

	StatePtr state = &start;

	for (int i = 1; i < argc; i++) {
		if (state) {
			state = state(argv[i], *this);
		} else if (mode != Params::modeError) {
			mode = Params::modeError;
			modeParam = L"Unknown parameter: ";
			modeParam2 = argv[i];
			break;
		} else {
			break;
		}
	}

	if (state && mode != Params::modeError)
		state(null, *this);
}
