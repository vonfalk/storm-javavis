#pragma once

#include "Utils/Exception.h"
#include "Utils/Object.h"

// The main header for the test framework. Gives the macros:
// CHECK(condition)
// CHECK_TITLE(condition, title)
// CHECK_EQ(expr, val)
// CHECK_EQ_TITLE(expr, val, title)
// BEGIN_TEST(name)
// END_TEST
// And the types:
// TestResult
// Test

class TestResult {
public:
	TestResult() : total(0), failed(0), crashed(0) {}

	nat total;
	nat failed;
	nat crashed;

	inline nat passed() const {
		return total - failed - crashed;
	}

	inline TestResult &operator +=(const TestResult &other) {
		total += other.total;
		failed += other.failed;
		crashed += other.crashed;
		return *this;
	}

	friend std::wostream &operator <<(std::wostream &to, const TestResult &r);
};

inline std::wostream &operator <<(std::wostream &to, const TestResult &r) {
	to << L"Passed " << r.passed() << L" of " << r.total << L" tests";
	if (r.failed > 0)
		to << L"\nFailed " << r.failed << L" tests!";
	if (r.crashed > 0)
		to << L"\nCrashed " << r.crashed << L" tests!";
	return to;
}

class Test;

class Tests : Singleton {
public:
	inline Tests() : only(null) {}

	//Run all tests, returns the statistics.
	static TestResult run();

private:
	typedef map<String, Test *> TestMap;
	TestMap tests;

	Test *only;

	static Tests &instance();
	static void addTest(Test *t, bool single); //Takes ownership of the test.

	friend class Test;
};

//Base class for all the tests. Keeps track of instances.
class Test : NoCopy {
public:
	virtual TestResult run() const = 0;

	inline const String &getName() const { return name; }
protected:
	Test(const String &name, bool single=false) : name(name) {
		Tests::instance().addTest(this, single);
	}

private:
	String name;
};


//////////////////////////////////////////////////////////////////////////
// Helper functions
//////////////////////////////////////////////////////////////////////////

template <class T, class U>
void verifyEq(TestResult &r, const T &lhs, const U &rhs, const String &expr) {
	if (!(lhs == rhs)) {
		r.failed++;
		std::wcout << L"Failed: " << expr << L" == " << lhs << L" != " << rhs << std::endl;
	}
}

#define OUTPUT_ERROR(expr, error) \
	std::wcout << L"Crashed " << expr << L": " << error << std::endl; \
	__result__.crashed++

#define DEFINE_TEST(name, single) \
class name : public Test { \
public: \
	virtual TestResult run() const; \
private: \
    name() : Test(_T(#name), single) {}	\
	static name instance; \
}


//////////////////////////////////////////////////////////////////////////
// Macros
//////////////////////////////////////////////////////////////////////////

#define CHECK_TITLE(expr, title) \
	try { \
		__result__.total++; \
		if (!(expr)) { \
			__result__.failed++; \
			std::wcout << "Failed: " << title << std::endl; \
		} \
	} catch (const Exception &e) {	\
		OUTPUT_ERROR(title, e.what()); \
	} catch (...) { \
		OUTPUT_ERROR(title, "unknown crash"); \
	}

#define CHECK_EQ_TITLE(expr, eq, title) \
	try { \
		__result__.total++; \
		std::wostringstream __stream__; \
		__stream__ << title; \
		verifyEq(__result__, expr, eq, __stream__.str()); \
	} catch (const Exception &e) { \
		OUTPUT_ERROR(title, e.what()); \
	} catch (...) { \
		OUTPUT_ERROR(title, "unknown crash"); \
	}

#define CHECK(expr) CHECK_TITLE(expr, #expr)

#define CHECK_EQ(expr, eq) CHECK_EQ_TITLE(expr, eq, #expr)

#define CHECK_ERROR(expr)						\
	try {										\
		__result__.total++;						\
		expr;									\
		__result__.failed++;					\
	} catch (const Exception &) {				\
	}


#define BEGIN_TEST(name) \
	DEFINE_TEST(name, false); \
	name name::instance; \
	TestResult name::run() const {\
		TestResult __result__; do

#define BEGIN_TEST_(name) \
	DEFINE_TEST(name, true); \
	name name::instance; \
	TestResult name::run() const { \
	    TestResult __result__; do

#define END_TEST \
	while (false); \
	return __result__; \
	}
