#include "stdafx.h"
#include "Params.h"
#include "Compiler/Server/Main.h"
#include "Compiler/Exception.h"
#include "Compiler/Engine.h"
#include "Compiler/Repl.h"
#include "Core/Timing.h"
#include "Core/Io/StdStream.h"
#include "Core/Io/Text.h"

void runRepl(Engine &e, const wchar *lang, Repl *repl) {
	TextReader *input = e.stdIn();
	TextWriter *output = e.stdOut();

	Str *line = null;
	while (!repl->exit()) {
		{
			StrBuf *prompt = new (e) StrBuf();
			if (line)
				*prompt << L"? ";
			else
				*prompt << lang << L"> ";

			output->write(prompt->toS());
			output->flush();
		}

		Str *data = input->readLine();
		if (!line) {
			line = data;
		} else {
			StrBuf *to = new (repl) StrBuf();
			*to << line << L"\n" << data;
			line = to->toS();
		}

		try {
			if (repl->eval(line))
				line = null;
		} catch (const Exception &err) {
			// TODO: Fix this whenever we have proper exceptions in Storm!
			std::wostringstream t;
			t << err << endl;
			output->writeLine(new (e) Str(t.str().c_str()));
			line = null;
		}
	}
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
		runRepl(e, lang, repl);
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

	Engine e(root, Engine::reuseMain);
	Moment end;

	try {
		switch (p.mode) {
		case Params::modeRepl:
			wcout << L"Welcome to the Storm compiler!" << endl;
			wcout << L"Root directory: " << root << endl;
			wcout << L"Compiler boot in " << (end - start) << endl;
			return runRepl(e, p.modeParam, p.modeParam2);
		case Params::modeFunction:
			return runFunction(e, p.modeParam);
		case Params::modeServer:
			server::run(e, proc::in(e), proc::out(e));
			return 0;
		default:
			throw InternalError(L"Unknown mode.");
		}
	} catch (const Exception &e) {
		// Sometimes, we need to print the exception before the engine is destroyed.
		wcerr << e << endl;
		return 1;
	}

	// Allow 1 s for all UThreads on the Compiler thread to terminate.
	Moment waitStart;
	while (os::UThread::leave() && Moment() - waitStart > s(1))
		;
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
