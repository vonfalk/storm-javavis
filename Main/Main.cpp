#include "stdafx.h"
#include "Params.h"
#include "Compiler/Exception.h"
#include "Compiler/Engine.h"
#include "Compiler/Repl.h"
#include "Core/Timing.h"

using namespace std;
using namespace storm;

// Read that is not locking up the compiler loop. Should be implemeted better when there
// is "real" IO in storm!
static os::ThreadGroup *ioGroup = null;
static os::Thread *ioThread = null;

void ioThreadMain() {
	// We use 'uThreads' to synchronize this!
}

bool ioRead(String &to) {
	getline(wcin, to);
	if (!wcin)
		return false;
	else
		return true;
}

void stopIo() {
	// Allow all our UThreads to exit.
	while (os::UThread::leave());

	if (ioThread) {
		del(ioThread);
		// Let the io thread exit cleanly.
		ioGroup->join();
		del(ioGroup);
	}
}

void startIo() {
	if (ioThread)
		return;
	ioGroup = new os::ThreadGroup();
	ioThread = new os::Thread(os::Thread::spawn(util::simpleVoidFn(&ioThreadMain), *ioGroup));
}

bool readLine(String &to) {
	startIo();
	os::Future<bool> fut;
	os::FnParams p;
	String *t = &to;
	p.add(t);
	os::UThread::spawn(&ioRead, false, p, fut, ioThread);
	return fut.result();
}


void runRepl(const wchar *lang, Repl *repl) {
	startIo();

	Str *line = null;
	while (!repl->exit()) {
		if (line)
			wcout << L"? ";
		else
			wcout << lang << L"> ";

		String data;
		if (!readLine(data))
			break;

		if (!line) {
			line = new (repl) Str(data.c_str());
		} else {
			StrBuf *to = new (repl) StrBuf();
			*to << line << L"\n" << new (repl) Str(data.c_str());
			line = to->toS();
		}

		try {
			if (repl->eval(line))
				line = null;
		} catch (const Exception &e) {
			wcout << e << endl;
			line = null;
		}
	}

	stopIo();
}

int runRepl(Engine &e, const wchar *lang, const wchar *input) {
	if (!lang)
		lang = L"bs";

	Name *replName = new (e) Name();
	replName->add(new (e) Str(L"lang"));
	replName->add(new (e) Str(lang));
	replName->add(new (e) Str(L"repl"));
	Function *replFn = as<Function>(e.scope().find(replName));
	if (!replFn) {
		wcerr << L"Could not find a repl for " << lang << L": no function named " << replName << L" exists." << endl;
		return 1;
	}

	Value replVal(Repl::stormType(e));
	if (!replVal.canStore(replFn->result)) {
		wcerr << L"The function " << replName << L" must return a subtype of "
			  << replVal << L", not " << replFn->result << endl;
		return 1;
	}

	typedef Repl *(*CreateRepl)();
	CreateRepl createRepl = (CreateRepl)replFn->ref().address();
	Repl *repl = (*createRepl)();

	if (input) {
		if (!repl->eval(new (e) Str(input))) {
			wcerr << L"The input given to the REPL does not represent a complete input. Try again!" << endl;
			return 1;
		}
	} else {
		runRepl(lang, repl);
	}

	return 0;
}

int runFunction(Engine &e, const wchar *function) {
	SimpleName *name = parseSimpleName(e, function);
	Named *found = e.scope().find(name);
	if (!found) {
		wcerr << L"Could not find " << function << endl;
		return 1;
	}

	Function *fn = as<Function>(found);
	if (!fn) {
		wcout << function << L" is not a function." << endl;
		return 1;
	}

	Value r = fn->result;
	if (r.isValue()) {
		wcout << function << L" returns a value-type. This is not yet supported." << endl;
		wcout << L"Try running it through the REPL instead!" << endl;
		return 1;
	}

	// We can just ignore the return value...
	typedef void (*Fn)();
	Fn p = (Fn)fn->ref().address();
	(*p)();
	return 0;
}

void help(const wchar *cmd) {
	wcout << L"Usage: " << endl;
	wcout << cmd << L"                  - launch the default REPL." << endl;
	wcout << cmd << L" <language>       - launch the REPL for <language>." << endl;
	wcout << cmd << L" -f <function>    - run <function> then exit." << endl;
	wcout << cmd << L" -i <name> <path> - import package at <path> as <name>." << endl;
	wcout << cmd << L" -r <path>        - use <path> as the root path." << endl;
}

int stormMain(int argc, const wchar *argv[]) {
	Params p(argc, argv);

	switch (p.mode) {
	case Params::modeHelp:
		help(argv[0]);
		return 0;
	case Params::modeError:
		wcerr << L"Error in parameters: " << p.modeParam;
		if (p.modeParam2)
			wcerr << p.modeParam2;
		wcerr << endl;
		return 1;
	}

	Moment start;

	// Start an Engine. TODO: Do not depend on 'Path'.
#ifdef DEBUG
	Path root = Path::dbgRoot() + L"root";
#else
	Path root = Path::executable() + L"root";
#endif
	if (p.root) {
		root = Path(String(p.root));
		if (!root.isAbsolute())
			root = Path::cwd() + root;
	}

	wcout << L"Welcome to the Storm compiler!" << endl;
	wcout << L"Root directory: " << root << endl;

	Engine e(root, Engine::reuseMain);
	Moment end;

	wcout << L"Compiler boot in " << (end - start) << endl;

	try {
		switch (p.mode) {
		case Params::modeRepl:
			return runRepl(e, p.modeParam, p.modeParam2);
		case Params::modeFunction:
			return runFunction(e, p.modeParam);
		default:
			throw InternalError(L"Unknown mode.");
		}
	} catch (const Exception &e) {
		// Sometimes, we need to print the exception before the engine is destroyed.
		wcerr << e << endl;
		return 1;
	}
}

#ifdef WINDOWS

int _tmain(int argc, const wchar *argv[]) {
	try {
		return stormMain(argc, argv);
	} catch (const Exception &e) {
		wcerr << L"Unhandled exception: " << e << endl;
		return 1;
	}
}

#else
#error "Please implement this stub for UNIX as well!"
#endif
