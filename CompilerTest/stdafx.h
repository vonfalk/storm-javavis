#pragma once

#include "Utils/Utils.h"
#include "Compiler/Storm.h"
#include "Compiler/Engine.h"

using namespace storm;

extern Engine *gEngine;

#include "Test/Test.h"

/**
 * Define test suites so we can place tests in a somewhat cronological order, so when porting we can
 * start worrying about the low-level parts and then build ourselves up to the final thing.
 */

// Basic gc operation, scanning simple objects without anything strange.
SUITE(GcScan, 1);
// GC along with threads.
SUITE(GcThreads, 2);
// Basic operation of the runtime. No code generation yet.
SUITE(Runtime, 3);

// Stress tests (takes time).
SUITEX(Stress, 100);


/**
 * Helpers for comparing objects in Storm.
 */

template <class T, class U>
void verifyObjEq(TestResult &r, T *lhs, U *rhs, const String &expr) {
	if (!(lhs->equals(rhs))) {
		r.failed++;
		std::wcout << L"Failed: " << expr << L" == " << lhs << L" != " << rhs << std::endl;
	}
}

#define CHECK_OBJ_EQ_TITLE(expr, eq, title)						\
	try {														\
		__result__.total++;										\
		std::wostringstream __stream__;							\
		__stream__ << title;									\
		verifyObjEq(__result__, expr, eq, __stream__.str());	\
	} catch (const Exception &e) {								\
		OUTPUT_ERROR(title, e);									\
	} catch (...) {												\
		OUTPUT_ERROR(title, "unknown crash");					\
	}


#define CHECK_OBJ_EQ(expr, eq) CHECK_OBJ_EQ_TITLE(expr, eq, #expr)
