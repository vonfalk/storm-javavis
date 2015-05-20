#include "stdafx.h"

#include "Test/TestMgr.h"

Engine *gEngine = null;

int _tmain(int argc, _TCHAR* argv[])
{
	initDebug();

	Timestamp start;

	TestResult r;

	try {
		// Initialize our engine.
		Path root = Path::executable() + Path(L"../root/");
		Engine e(root, Engine::reuseMain);
		gEngine = &e;

		PLN("Compiler boot: " << (Timestamp() - start));

		r = Tests::run();

		gEngine = null;
		Timestamp tests;
	} catch (const Exception &e) {
		PLN("Creation error: " << e.what());
	}

	Timestamp end;
	PLN("Total time: " << (end - start));

	// Object::dumpLeaks(); // Done earlier, in ~Engine

	return r.ok() ? 0 : 1;
}

