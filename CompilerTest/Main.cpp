#include "stdafx.h"

#include "Test/TestMgr.h"
#include "Core/Timing.h"

Engine *gEngine = null;

int _tmain(int argc, _TCHAR *argv[]) {
	initDebug();

	Moment start;
	TestResult r;

	try {
		// Initialize our engine.
		Path root = Path::executable() + Path(L"../root/");

		Moment start;
		Engine e(root, Engine::reuseMain);
		gEngine = &e;

		PLN(L"Compiler boot: " << (Moment() - start));

		r = Tests::run();

		gEngine = null;
	} catch (const Exception &e) {
		PLN(L"Creation error: " << e.what());
	}

	Moment end;
	PLN(L"Total time: " << (end - start));

	return r.ok() ? 0 : 1;
}
