#include "stdafx.h"

#include "Test/Lib/TestMgr.h"
#include "Shared/Timing.h"

Engine *gEngine = null;

int _tmain(int argc, _TCHAR* argv[]) {
	initDebug();

	Moment start;

	TestResult r;

	try {
		// Initialize our engine.
		Path root = Path::executable() + Path(L"../root/");
		Engine e(root, Engine::reuseMain);
		gEngine = &e;

		PLN("Compiler boot: " << (Moment() - start));

		r = Tests::run();

		gEngine = null;
		Timestamp tests;
	} catch (const Exception &e) {
		PLN("Creation error: " << e.what());
	}

	Moment end;
	PLN("Total time: " << (end - start));

	// Object::dumpLeaks(); // Done earlier, in ~Engine

	return r.ok() ? 0 : 1;
}

