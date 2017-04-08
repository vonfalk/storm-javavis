#pragma once

#include "Utils/Utils.h"
#include "Core/Storm.h"
#include "Core/Str.h"
#include "Core/StrBuf.h"
#include "Compiler/Storm.h"
#include "Compiler/Engine.h"

using namespace storm;

// Get the global engine object.
Engine &gEngine();

// Get the global GC object.
Gc &gc();

#include "Test/Lib/Test.h"

/**
 * Define test suites so we can place tests in a somewhat cronological order, so when porting we can
 * start worrying about the low-level parts and then build ourselves up to the final thing.
 */


// Basic threading, no GC yet.
SUITE(OS, 0);
// Basic gc operation, scanning simple objects without anything strange.
SUITE_(GcScan, 1);
// Gc operation on Storm objects declared in C++.
SUITE(GcObjects, 2);
// GC along with threads.
SUITE(GcThreads, 3);
// Basic operation of the runtime. No code generation yet.
SUITE(Core, 4);
// Basic tests of the code generation backend. Further thests rely on these to work.
SUITE(CodeBasic, 5);
// Tests of the code generation backend.
SUITE(Code, 6);
// More involved tests of Storm. Still no compilation from sources.
SUITE(Storm, 7);
// Simple tests of Basic Storm
SUITE(SimpleBS, 8);
// More heavy tests of Basic Storm
SUITE(BS, 9);
// Syntax server logic.
SUITE(Server, 10);

// UI library. Sadly not too many automated tests.
SUITEX(Gui, 20);

// Stress tests (takes a couple of minutes, too slow to be included always).
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


// Call a function and check all extra register so they are properly preserved.
int callFn(const void *ptr, int v);
int64 callFn(const void *ptr, int64 v);


class Error : public Exception {
public:
	String what() const { return L"Test error"; }
};
