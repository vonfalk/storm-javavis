#include "stdafx.h"
#include "Params.h"
#include "Debug.h"
#include "Compiler/Server/Main.h"
#include "Compiler/Exception.h"
#include "Compiler/Engine.h"
#include "Compiler/Package.h"
#include "Compiler/Repl.h"
#include "Compiler/Version.h"
#include "Core/Timing.h"
#include "Core/Io/StdStream.h"
#include "Core/Io/Text.h"

void runRepl(Engine &e, const wchar_t *lang, Repl *repl) {
	TextInput *input = e.stdIn();
	TextOutput *output = e.stdOut();

	Str *line = null;
	while (true) {
		{
			StrBuf *prompt = new (e) StrBuf();
			if (line)
				*prompt << S("? ");
			else
				*prompt << lang << S("> ");

			output->write(prompt->toS());
			output->flush();
		}

		Str *data = input->readLine();
		if (!line) {
			line = data;
		} else {
			StrBuf *to = new (repl) StrBuf();
			*to << line << S("\n") << data;
			line = to->toS();
		}

		try {
			Repl::Result result = repl->eval(line, null);
			StrBuf *msg = new (repl) StrBuf();
			if (result.isSuccess()) {
				if (Str *r = result.result())
					*msg << S("=> ") << r;
			} else if (Str *e = result.isError()) {
				*msg << S("Error: ") << e;
			} else if (result.isTerminate()) {
				break;
			}

			if (!result.isIncomplete())
				line = null;

			output->writeLine(msg->toS());
		} catch (const storm::Exception *err) {
			output->writeLine(err->toS());
			line = null;
		} catch (const ::Exception &err) {
			std::wostringstream t;
			t << err << endl;
			output->writeLine(new (e) Str(t.str().c_str()));
			line = null;
		}
	}
}

int runRepl(Engine &e, const wchar_t *lang, const wchar_t *input) {
	if (!lang)
		lang = L"bs";

	Name *replName = new (e) Name();
	replName->add(new (e) Str(S("lang")));
	replName->add(new (e) Str(lang));
	replName->add(new (e) Str(S("repl")));
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

	typedef Repl *(CODECALL *CreateRepl)();
	CreateRepl createRepl = (CreateRepl)replFn->ref().address();
	Repl *repl = (*createRepl)();

	if (input) {
		Repl::Result result = repl->eval(new (e) Str(input), null);
		if (result.isSuccess()) {
			if (Str *r = result.result())
				wcerr << L"=> " << r << endl;
		} else if (Str *e = result.isError()) {
			wcerr << L"Error: " << e << endl;
			return 1;
		} else if (result.isIncomplete()) {
			return 1;
		}
	} else {
		repl->greet();
		runRepl(e, lang, repl);
	}

	return 0;
}

int runFunction(Engine &e, const wchar_t *function) {
	SimpleName *name = parseSimpleName(new (e) Str(function));
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
	if (!r.returnInReg()) {
		wcout << function << L" returns a value-type. This is not yet supported." << endl;
		wcout << L"Try running it through the REPL instead!" << endl;
		return 1;
	}

	// We can just ignore the return value...
	RunOn run = fn->runOn();
	const void *addr = fn->ref().address();
	if (run.state == RunOn::named) {
		os::FnCall<void, 1> call = os::fnCall();
		os::Future<void> result;
		os::Thread on = run.thread->thread()->thread();
		os::UThread::spawn(addr, false, call, result, &on);
		result.result();
	} else {
		typedef void (*Fn)();
		Fn p = (Fn)addr;
		(*p)();
	}
	return 0;
}

int runTests(Engine &e, const wchar_t *package, bool recursive) {
	SimpleName *name = parseSimpleName(new (e) Str(package));
	Named *found = e.scope().find(name);
	if (!found) {
		wcerr << L"Could not find " << package << endl;
		return 1;
	}

	Package *pkg = as<Package>(found);
	if (!pkg) {
		wcerr << package << L" is not a package." << endl;
		return 1;
	}

	SimpleName *testMain = parseSimpleName(e, S("test.runCmdline"));
	testMain->last()->params->push(Value(StormInfo<Package>::type(e)));
	testMain->last()->params->push(Value(StormInfo<Bool>::type(e)));
	Function *f = as<Function>(e.scope().find(testMain));
	if (!f) {
		wcerr << L"Could not find the test implementation's main function: " << testMain << endl;
		return 1;
	}

	if (f->result.type != StormInfo<Bool>::type(e)) {
		wcerr << L"The signature of the test implementation's main function ("
			  << testMain << L") is incorrect. Expected Bool, but got "
			  << f->result.type->identifier() << endl;
		return 1;
	}

	os::FnCall<Bool> c = os::fnCall().add(pkg).add(recursive);
	Bool ok = c.call(f->ref().address(), false);
	return ok ? 0 : 1;
}

void importPkgs(Engine &into, const Params &p) {
	for (Nat i = 0; i < p.import.size(); i++) {
		const Import &import = p.import[i];
		SimpleName *n = parseSimpleName(new (into) Str(import.into));
		if (n->empty())
			continue;

		NameSet *ns = into.nameSet(n->parent(), true);
		Url *path = parsePath(new (into) Str(import.path));
		if (!path->absolute())
			path = cwdUrl(into)->push(path);
		Package *pkg = new (into) Package(n->last()->name, path);
		ns->add(pkg);
	}
}

void showVersion(Engine &e) {
	Array<VersionTag *> *tags = storm::versions(e.package(S("core")));
	if (tags->empty()) {
		wcout << L"Error: No version information found. Make sure this release was compiled correctly." << std::endl;
		return;
	}

	VersionTag *best = tags->at(0);
	for (Nat i = 1; i < tags->count(); i++) {
		if (best->path()->count() > tags->at(i)->path()->count())
			best = tags->at(i);
	}

	wcout << best->version << std::endl;
}

int stormMain(int argc, const wchar_t *argv[]) {
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

	Engine e(root, Engine::reuseMain, &argv);
	Moment end;

	importPkgs(e, p);

	int result = 1;

	try {
		switch (p.mode) {
		case Params::modeRepl:
			if (p.modeParam2 == null) {
				wcout << L"Welcome to the Storm compiler!" << endl;
				wcout << L"Root directory: " << root << endl;
				wcout << L"Compiler boot in " << (end - start) << endl;
			}
			result = runRepl(e, p.modeParam, p.modeParam2);
			break;
		case Params::modeFunction:
			result = runFunction(e, p.modeParam);
			break;
		case Params::modeTests:
			result = runTests(e, p.modeParam, false);
			break;
		case Params::modeTestsRec:
			result = runTests(e, p.modeParam, true);
			break;
		case Params::modeVersion:
			showVersion(e);
			result = 0;
			break;
		case Params::modeServer:
			server::run(e, proc::in(e), proc::out(e));
			result = 0;
			break;
		default:
			throw new (e) InternalError(S("Unknown mode."));
		}
	} catch (const storm::Exception *e) {
		wcerr << e << endl;
		return 1;
	} catch (const ::Exception &e) {
		// Sometimes, we need to print the exception before the engine is destroyed.
		wcerr << e << endl;
		return 1;
	}

	// Allow 1 s for all UThreads on the Compiler thread to terminate.
	Moment waitStart;
	while (os::UThread::leave() && Moment() - waitStart > time::s(1))
		;
	return result;
}

#ifdef WINDOWS

int _tmain(int argc, const wchar *argv[]) {
	try {
		return stormMain(argc, argv);
	} catch (const ::Exception &e) {
		wcerr << L"Unhandled exception: " << e << endl;
		return 1;
	}
}

#else

int main(int argc, const char *argv[]) {
	try {
		vector<String> args(argv, argv+argc);
		vector<const wchar_t *> c_args(argc);
		for (int i = 0; i < argc; i++)
			c_args[i] = args[i].c_str();

		return stormMain(argc, &c_args[0]);
	} catch (const ::Exception &e) {
		wcerr << "Unhandled exception: " << e << endl;
		return 1;
	}
}

#endif
