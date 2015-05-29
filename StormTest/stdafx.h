#pragma once

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>

#include "Utils/Utils.h"
#include "Lib/Storm.h"
#include "Storm/Exception.h"
#include "Storm/Engine.h"
#include "Storm/Std.h"
#include "Storm/Lib/Array.h"
#include "Storm/Lib/Debug.h"
#include "Storm/Io/Url.h"

#include "Exception.h"

using namespace storm;

extern Engine *gEngine;

#include "Test/Test.h"

template <class T, class U>
void verifyObjEq(TestResult &r, Auto<T> lhs, Auto<U> rhs, const String &expr) {
	if (!(lhs->equals(rhs))) {
		r.failed++;
		std::wcout << L"Failed: " << expr << L" == " << lhs << L" != " << rhs << std::endl;
	}
}

template <class T>
Auto<T> tCapture(Auto<T> t) {
	return t;
}

template <class T>
Auto<T> tCapture(T *v) {
	return Auto<T>(v);
}

#define CHECK_OBJ_EQ_TITLE(expr, eq, title)								\
	try {																\
		__result__.total++;												\
		std::wostringstream __stream__;									\
		__stream__ << title;											\
		verifyObjEq(__result__, tCapture(expr), tCapture(eq), __stream__.str()); \
	} catch (const Exception &e) {										\
		OUTPUT_ERROR(title, e);											\
	} catch (...) {														\
		OUTPUT_ERROR(title, "unknown crash");							\
	}

#define CHECK_OBJ_EQ(expr, eq) CHECK_OBJ_EQ_TITLE(expr, eq, #expr)
