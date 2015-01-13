#include "stdafx.h"

#include "Test/TestMgr.h"

Engine *gEngine = null;

int _tmain(int argc, _TCHAR* argv[])
{
	initDebug();

	Timestamp start;

	try {
		// Initialize our engine.
		Path root = Path::executable() + Path(L"../root/");
		Engine e(root);
		gEngine = &e;

		Tests::run();

		gEngine = null;
	} catch (const Exception &e) {
		PLN("Creation error: " << e.what());
	}

	Timestamp end;
	PLN("Total time: " << (end - start));

	Object::dumpLeaks();

	return 0;
}

