#include "stdafx.h"
#include "Utils/Timestamp.h"
#include "Utils/Exception.h"
#include "Storm/Engine.h"
#include "Storm/Repl.h"
#include "Storm/Type.h"
#include "Shared/Str.h"
#include "Storm/Function.h"

using namespace std;
using namespace storm;

// Read that is not locking up the compiler loop. Should be implemeted better when there
// is "real" IO in storm!
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
		Sleep(200);
	}
}

void startIo() {
	if (ioThread)
		return;
	ioThread = new os::Thread(os::Thread::spawn(simpleVoidFn(&ioThreadMain)));
}

bool readLine(String &to) {
	startIo();
	os::Future<bool> fut;
	os::FnParams p;
	p.add(&to);
	os::UThread::spawn(&ioRead, false, p, fut, ioThread);
	return fut.result();
}

void runMainLoop(Engine &engine, const String &lang, Par<LangRepl> repl) {
	Auto<Str> line;
	while (!repl->exit()) {
		if (line)
			wcout << L"? ";
		else
			wcout << lang << L"> ";

		String data;
		if (!readLine(data))
			break;

		if (!line)
			line = CREATE(Str, engine, data);
		else
			line = CREATE(Str, engine, line->v + L"\n" + data);

		try {
			if (repl->eval(line))
				line = Auto<Str>();
		} catch (const Exception &e) {
			wcout << L"Unhandled exception from 'eval': " << e << endl;
			line = Auto<Str>();
		}
	}
}

// Launch a main loop.
int launchMainLoop(Engine &engine, const String &lang, const String &command) {
	// Launch a main loop!
	Auto<Name> replName = CREATE(Name, engine);
	replName->add(L"lang");
	replName->add(lang);
	replName->add(L"Repl");
	Type *replType = as<Type>(engine.scope()->find(replName));
	if (!replType) {
		wcout << L"Could not find a Repl class for " << lang << L" (" << replName << L")" << endl;
		return 1;
	}

	Type *langRepl = LangRepl::stormType(engine);
	if (!replType->isA(langRepl)) {
		wcout << L"The type " << replName << L" needs to inherit from " << langRepl->identifier() << endl;
		return 1;
	}

	Function *replCtor = as<Function>(replType->findCpp(Type::CTOR, vector<Value>(1, Value::thisPtr(replType))));
	if (!replCtor) {
		wcout << L"No suitable constructor found for " << replName << L"." << endl;
		return 1;
	}

	Auto<LangRepl> repl = create<LangRepl>(replCtor, os::FnParams());

	if (command == L"") {
		runMainLoop(engine, lang, repl);
	} else {
		repl->eval(steal(CREATE(Str, engine, command)));
	}

	return 0;
}

int launchFn(Engine &e, const String &fnName) {
	Auto<Name> name = parseSimpleName(e, fnName);
	Named *named = e.scope()->find(name);
	if (!named) {
		wcout << L"Could not find " << fnName << L"!" << endl;
		return 1;
	}

	Function *fn = as<Function>(named);
	if (!fn) {
		wcout << fnName << L" is not a function." << endl;
		return 1;
	}

	Value r = fn->result;
	if (r.isBuiltIn()) {
		// We can just ignore the return value.
		fn->call<void>();
	} else if (r.isValue()) {
		wcout << L"Functions returning values are not supported yet." << endl;
		return 1;
	} else if (r.isClass()) {
		Auto<Object> o = fn->call<Object *>();
	} else {
		wcout << L"Unknown return type for the function." << endl;
		return 1;
	}

	return 0;
}

enum LaunchMode {
	launchHelp,
	launchRepl,
	launchFunction,
};

struct Launch {
	LaunchMode mode;

	// Root path.
	Path root;

	// Which function to launch?
	String fn;

	// Which language's repl to launch?
	String repl;

	// Repl command (if any).
	String replCmd;
};

void help() {
	wcout << L"Usage:" << endl;
	wcout << L"storm               - launch the default REPL." << endl;
	wcout << L"storm <language>    - launc the REPL for <language>" << endl;
	wcout << L"storm -f <function> - run <function>, then exit." << endl;
}

struct StatePtr;
typedef StatePtr (*State)(const String &, Launch &);

struct StatePtr {
	StatePtr() : p(null) {}
    StatePtr(State p) : p(p) {}
	StatePtr operator () (const String &s, Launch &l) { return (*p)(s, l); }
	operator bool() { return p != null; }
    State p;
};

StatePtr parseStart(const String &arg, Launch &result);

StatePtr parseFunction(const String &arg, Launch &result) {
	result.fn = arg;
	return &parseStart;
}

StatePtr parseReplCommand(const String &arg, Launch &result) {
	result.replCmd = arg;
	return &parseStart;
}

StatePtr parseRoot(const String &arg, Launch &result) {
	Path p(arg);
	if (p.isAbsolute())
		result.root = p;
	else
		result.root = Path::cwd() + p;
	return &parseStart;
}

StatePtr parseStart(const String &arg, Launch &result) {
	if (arg == L"-?") {
		result.mode = launchHelp;
	} else if (arg == L"-f") {
		result.mode = launchFunction;
		return &parseFunction;
	} else if (arg == L"-c") {
		return &parseReplCommand;
	} else if (arg == L"-r") {
		return &parseRoot;
	} else {
		result.mode = launchRepl;
		result.repl = arg;
		return &parseStart;
	}

	return StatePtr();
}

bool parseParams(int argc, wchar *argv[], Launch &result) {
	result.mode = launchRepl;
	result.repl = L"bs";
	StatePtr state = &parseStart;

	for (int i = 1; i < argc; i++) {
		String arg = argv[i];
		if (state) {
			state = state(arg, result);
		} else {
			wcout << L"Unknown parameter: " << arg << endl;
			return false;
		}
	}

	return true;
}


int _tmain(int argc, _TCHAR* argv[]) {
	initDebug();

	try {
		Launch launch;
#ifdef DEBUG
		launch.root = Path::dbgRoot() + L"root";
#else
		launch.root = Path::executable() + L"root";
#endif

		if (!parseParams(argc, argv, launch)) {
			wcout << L"Error parsing command-line parameters." << endl;
			help();
			return 1;
		}

		wcout << L"Welcome to the Storm compiler!" << endl;

		startIo();

		wcout << L"Root directory: " << launch.root << endl;

		Timestamp start;
		Engine engine(launch.root, Engine::reuseMain);
		Timestamp end;

		wcout << L"Compiler boot in " << (end - start) << endl;

		try {
			int r = 0;

			switch (launch.mode) {
			case launchHelp:
				help();
				r = 0;
				break;
			case launchRepl:
				r = launchMainLoop(engine, launch.repl, launch.replCmd);
				break;
			case launchFunction:
				r = launchFn(engine, launch.fn);
				break;
			default:
				wcout << L"Unknown launch mode. Exiting..." << endl;
				r = 1;
				break;
			}

			stopIo();
			return r;
		} catch (const Exception &e) {
			// We need an extra so that errrors depending on the engine will not crash at least!";
			wcout << L"Unhandled exception: " << e << endl;
			stopIo();
			return 2;
		}
	} catch (const Exception &e) {
		wcout << L"Unhandled exception: " << e << endl;
		stopIo();
		return 2;
	}
}

