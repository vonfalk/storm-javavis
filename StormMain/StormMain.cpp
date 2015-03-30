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

	Type *langRepl = LangRepl::type(engine);
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
		getline(wcin, data);
		if (!wcin)
			break;

		if (!line)
			line = CREATE(Str, engine, data);
		else
			line = CREATE(Str, engine, line->v + L"\n" + data);

		if (repl->eval(line))
			line = Auto<Str>();
	}

	return 0;
}


int _tmain(int argc, _TCHAR* argv[]) {
	try {
		wcout << L"Welcome to the Storm compiler!" << endl;

		Path path = Path::dbgRoot() + L"root";
		wcout << L"Root directory: " << path << endl;

		String lang = L"bs";
		if (argc >= 2)
			lang = argv[1];

		Timestamp start;
		Engine engine(path, Engine::reuseMain);
		Timestamp end;

		wcout << L"Compiler boot in " << (end - start) << endl;

		// Todo: We probably want more launch options here, for example run a specific
		// function as our main and so on...
		return launchMainLoop(engine, lang);
	} catch (const Exception &e) {
		wcout << L"Unhandled exception: " << e << endl;
		return 2;
	}
}

