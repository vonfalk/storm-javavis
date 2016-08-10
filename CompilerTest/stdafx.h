#pragma once

#include "Utils/Utils.h"
#include "Compiler/Storm.h"
#include "Compiler/Engine.h"

using namespace storm;

extern Engine *gEngine;

#include "Test/Test.h"

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
