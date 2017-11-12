#include "stdafx.h"
#include "Test.h"
#include "Core/Str.h"
#include "Core/Thread.h"

namespace testlib {

	STORM_DEFINE_THREAD(LibThread);

	Int test(Int v) {
		return v + 1;
	}

	Int test(Str *v) {
		if (runtime::typeOf(v) != Str::stormType(v->engine()))
			return 0;
		else
			return 1;
	}

	Int test(Array<Int> *v) {
		if (runtime::typeOf(v) != Array<Int>::stormType(v->engine()))
			return 0;

		Int r = 0;
		for (Nat i = 0; i < v->count(); i++)
			r += v->at(i);
		return r;
	}

	Thread *testThread(EnginePtr e, Int v) {
		switch (v) {
		case 0:
			return Compiler::thread(e.v);
		default:
			return LibThread::thread(e.v);
		}
	}
}
