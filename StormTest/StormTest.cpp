#include "stdafx.h"

#include "Test/TestMgr.h"

Engine *gEngine = null;

int _tmain(int argc, _TCHAR* argv[])
{
	initDebug();

	Sleep(2000);

	Timestamp start;

	try {
		// Initialize our engine.
		Path root = Path::executable() + Path(L"../root/");
		Engine e(root);
		gEngine = &e;

		PLN("Compiler boot: " << (Timestamp() - start));

		Tests::run();

		gEngine = null;
		Timestamp tests;
	} catch (const Exception &e) {
		PLN("Creation error: " << e.what());
	}

	Timestamp end;
	PLN("Total time: " << (end - start));

	Object::dumpLeaks();

	return 0;
}

