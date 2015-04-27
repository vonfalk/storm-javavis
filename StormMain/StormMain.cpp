#include "stdafx.h"

#include "Utils/Timestamp.h"
#include "Utils/Exception.h"
#include "Storm/Engine.h"
#include "Storm/Repl.h"
#include "Storm/Type.h"
#include "Storm/Lib/Str.h"
#include "Storm/Function.h"

using namespace std;
using namespace storm;

// Read that is not locking up the compiler loop. Should be implemeted better when there
// is "real" IO in storm!
static code::Thread *ioThread = null;

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
	if (ioThread) {
		del(ioThread);
		// Let the io thread exit cleanly.
		Sleep(200);
	}
}

void startIo() {
	if (ioThread)
		return;
	ioThread = new code::Thread(code::Thread::spawn(simpleVoidFn(&ioThreadMain)));
}

bool readLine(String &to) {
	startIo();
	code::Future<bool> fut;
	code::FnParams p;
	p.add(&to);
	code::UThread::spawn(&ioRead, false, p, fut, ioThread);
	return fut.result();
}

// Launch a main loop.
int launchMainLoop(Engine &engine, const String &lang) {
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

	Function *replCtor = as<Function>(replType->find(Type::CTOR, vector<Value>(1, Value::thisPtr(replType))));
	if (!replCtor) {
		wcout << L"No suitable constructor found for " << replName << L"." << endl;
		return 1;
	}

	Auto<LangRepl> repl = create<LangRepl>(replCtor, code::FnParams());

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

	// Allow all our UThreads to exit.
	while (code::UThread::leave());

	return 0;
}


int _tmain(int argc, _TCHAR* argv[]) {
	try {
		wcout << L"Welcome to the Storm compiler!" << endl;

		startIo();

#ifdef DEBUG
		Path path = Path::dbgRoot() + L"root";
#else
		Path path = Path::executable().parent() + L"root";
#endif
		wcout << L"Root directory: " << path << endl;

		String lang = L"bs";
		if (argc >= 2)
			lang = argv[1];

		Timestamp start;
		Engine engine(path, Engine::reuseMain);
		Timestamp end;

		wcout << L"Compiler boot in " << (end - start) << endl;

		try {
			// Todo: We probably want more launch options here, for example run a specific
			// function as our main and so on...
			int r = launchMainLoop(engine, lang);
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

