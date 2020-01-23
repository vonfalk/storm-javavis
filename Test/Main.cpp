#include "stdafx.h"
#include "Test/Lib/TestMgr.h"
#include "Core/Timing.h"
#include "OS/Thread.h"

static Engine *engineObj = null;
static void *stackBase = null;

Engine &gEngine() {
	if (!engineObj) {
		PLN(L"==> Starting compiler...");
		Moment start;
		Path root = Path::executable() + Path(L"../root/");
		engineObj = new Engine(root, Engine::reuseMain, stackBase);
		PLN(L"==> Compiler boot: " << (Moment() - start));
	}
	return *engineObj;
}

static Gc *usedGc = null;

Gc &gc() {
	if (engineObj) {
		if (usedGc) {
			delete usedGc;
			usedGc = null;
		}
		return engineObj->gc;
	}

	if (!usedGc) {
		// Don't take up too much VM since we have to share VM with the Engine later on.
		usedGc = new Gc(2*1024*1024, 1000);
		usedGc->attachThread();
	}
	return *usedGc;
}

int _tmain(int argc, const wchar_t *argv[]) {
	initDebug();

	stackBase = &argv;
	// Allows using threads without an Engine.
	os::Thread::setStackBase(stackBase);

	Moment start, end;
	TestResult r;

	try {
		start = Moment();
		r = Tests::run(argc, argv);
		end = Moment();

		delete engineObj;
		engineObj = null;
		delete usedGc;
		usedGc = null;
	} catch (const storm::Exception *e) {
		PLN(L"Unknown error: " << e);
	} catch (const ::Exception &e) {
		PLN(L"Unknown error: " << e.what());
	}

	PLN(L"Total time: " << (end - start));

	return r.ok() ? 0 : 1;
}
