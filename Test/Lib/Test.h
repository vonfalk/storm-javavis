#pragma once

#include "Utils/Utils.h"
#include "Utils/Exception.h"
#include "Utils/Object.h"

// The main header for the test framework. Gives the macros:
// CHECK(condition)
// CHECK_TITLE(condition, title)
// CHECK_EQ(expr, val)
// CHECK_NEQ(expr, val)
// CHECK_EQ_TITLE(expr, val, title)
// CHECK_NEQ_TITLE(expr, val, title)
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

	inline bool ok() const {
		return failed == 0 && crashed == 0;
	}

	friend std::wostream &operator <<(std::wostream &to, const TestResult &r);
};

inline std::wostream &operator <<(std::wostream &to, const TestResult &r) {
	to << L"Passed ";
	if (r.passed() == r.total)
		to << L"all ";
	else
		to << r.passed() << L" of ";
	to << r.total << L" tests";
	if (r.failed > 0)
		to << L"\nFailed " << r.failed << L" tests!";
	if (r.crashed > 0)
		to << L"\nCrashed " << r.crashed << L" tests!";
	return to;
}

class Test;
class Suite;

class Tests : Singleton {
public:
	inline Tests() : singleSuite(null), singleTest(null) {}

	// Run all tests, returns the statistics.
	static TestResult run();

private:
	typedef map<int, Suite *> SuiteMap;
	SuiteMap suites;

	typedef map<String, Test *> TestMap;
	TestMap tests;

	bool singleSuite;
	bool singleTest;

	static Tests &instance();
	static void addTest(Test *t, bool single);
	static void addSuite(Suite *t, bool single);

	friend class Test;
	friend class Suite;

	void runTests(TestResult &result);
	void runSuite(Suite *s, TestResult &result);
	int countSuite(Suite *s);
};

// Base class for all test suites.
class Suite : NoCopy {
public:
	const String name;
	const int order;
	const bool single;

protected:
	Suite(const String &name, int order, bool single, bool ignore) : name(name), order(order), single(single) {
		if (!ignore) {
			Tests::instance().addSuite(this, single);
		}
	}
};

// Base class for all the tests. Keeps track of instances.
class Test : NoCopy {
public:
	virtual TestResult run() const = 0;

	const String name;
	Suite *const suite;
	const bool single;

protected:
	Test(const String &name, Suite *suite = null, bool single = false) : name(name), suite(suite), single(single) {
		Tests::instance().addTest(this, single);
	}
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

template <class T, class U>
void verifyNeq(TestResult &r, const T &lhs, const U &rhs, const String &expr) {
	if (lhs == rhs) {
		r.failed++;
		std::wcout << L"Failed: " << expr << L" != " << lhs << L" == " << rhs << std::endl;
	}
}

#define OUTPUT_ERROR(expr, error)									  \
	std::wcout << L"Crashed " << expr << L": " << error << std::endl; \
	__result__.crashed++

#define DEFINE_SUITE(name, order, single, disable)			\
	class name : public Suite {								\
	private:												\
	name() : Suite(WIDEN(#name), order, single, disable) {}	\
	public:													\
	static Suite &instance() {								\
		static name s;										\
		return s;											\
	}														\
	}

#define EXPAND(x) x // Hack for msvc
#define PICK_FIRST(a, ...) a
#define PICK_NO_3(a, b, c, ...) c
#define NUM_ARGS(...) EXPAND(PICK_NO_3(__VA_ARGS__, 1, 0))

#define SUITE_OR_NULL_0(dummy)     null
#define SUITE_OR_NULL_1(dummy, type) &type::instance()

#define SUITE_OR_NULL3(cond, ...) EXPAND(SUITE_OR_NULL_ ## cond (__VA_ARGS__))
#define SUITE_OR_NULL2(cond, ...) SUITE_OR_NULL3(cond, __VA_ARGS__)
#define SUITE_OR_NULL(...) SUITE_OR_NULL2(NUM_ARGS(__VA_ARGS__), __VA_ARGS__)

// Name is the first parameter. It has to be inside of VA_ARGS for SUITE_OR_NULL to work...
#define DEFINE_TEST(name, suite, single)			\
	class name : public Test {						\
	public:											\
	virtual TestResult run() const;					\
	private:										\
	name() : Test(STRING(name), suite, single) {}	\
	static name instance;							\
	}


//////////////////////////////////////////////////////////////////////////
// Macros
//////////////////////////////////////////////////////////////////////////

#define CHECK_TITLE(expr, title)							\
	try {													\
		__result__.total++;									\
		if (!(expr)) {										\
			__result__.failed++;							\
			std::wcout << "Failed: " << title << std::endl; \
		}													\
	} catch (const Exception &e) {							\
		OUTPUT_ERROR(title, e);								\
	} catch (...) {											\
		OUTPUT_ERROR(title, "unknown crash");				\
	}

#define CHECK_EQ_TITLE(expr, eq, title)					  \
	try {												  \
		__result__.total++;								  \
		std::wostringstream __stream__;					  \
		__stream__ << title;							  \
		verifyEq(__result__, expr, eq, __stream__.str()); \
	} catch (const Exception &e) {						  \
		OUTPUT_ERROR(title, e);							  \
	} catch (...) {										  \
		OUTPUT_ERROR(title, "unknown crash");			  \
	}

#define CHECK_NEQ_TITLE(expr, eq, title)					\
	try {													\
		__result__.total++;									\
		std::wostringstream __stream__;						\
		__stream__ << title;								\
		verifyNeq(__result__, expr, eq, __stream__.str());	\
	} catch (const Exception &e) {							\
		OUTPUT_ERROR(title, e);								\
	} catch (...) {											\
		OUTPUT_ERROR(title, "unknown crash");				\
	}

#define CHECK(expr) CHECK_TITLE(expr, #expr)

#define CHECK_EQ(expr, eq) CHECK_EQ_TITLE(expr, eq, #expr)

#define CHECK_NEQ(expr, eq) CHECK_NEQ_TITLE(expr, eq, #expr)

#define CHECK_ERROR(expr, type)											\
	try {																\
		__result__.total++;												\
		expr;															\
		__result__.failed++;											\
		std::wcout << "Failed: " << #expr << ", did not fail as expected" << std::endl; \
	} catch (const type &) {											\
	} catch (const Exception &e) {										\
		std::wcout << "Failed: " << #expr << ", did not throw " << #type << " as expected." << std::endl; \
		std::wcout << e << std::endl;									\
		__result__.failed++;											\
	} catch (...) {														\
		OUTPUT_ERROR(#expr, "unknown error");							\
	}

#define CHECK_RUNS(expr)						\
	try {										\
		__result__.total++;						\
		expr;									\
	} catch (const Exception &e) {				\
		OUTPUT_ERROR(#expr, e);					\
	} catch (...) {								\
		OUTPUT_ERROR(#expr, "unknown crash");	\
	}

#define SUITE(name, order)						\
	DEFINE_SUITE(name, order, false, false)

#define SUITE_(name, order)						\
	DEFINE_SUITE(name, order, true, false)

#define SUITEX(name, order)						\
	DEFINE_SUITE(name, order, false, true)

#define BEGIN_TEST_HELP(name, suite, single)	\
	DEFINE_TEST(name, suite, single);			\
	name name::instance;						\
	TestResult name::run() const {				\
	TestResult __result__; do

// Parameters: name, suite (suite is optional).
#define BEGIN_TEST(...)											\
	BEGIN_TEST_HELP(PICK_FIRST(__VA_ARGS__), SUITE_OR_NULL(__VA_ARGS__), false)

// Parameters: name, suite (suite is optional).
#define BEGIN_TEST_(...)										\
	BEGIN_TEST_HELP(PICK_FIRST(__VA_ARGS__), SUITE_OR_NULL(__VA_ARGS__), true)

#define BEGIN_TESTX(name, ...)					\
	TestResult name() {							\
	TestResult __result__; do

#define END_TEST	   \
	while (false);	   \
	return __result__; \
	}

#define BEGIN_TEST_FN(name, ...)				\
	void name(TestResult &__result__, __VA_ARGS__) { do

#define END_TEST_FN								\
	while (false);								\
	}

#define CALL_TEST_FN(name, ...)					\
	name(__result__, __VA_ARGS__)