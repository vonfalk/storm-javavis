#pragma once

#include "Utils/Utils.h"
#include "Utils/Exception.h"
#include "Utils/Object.h"

// The main header for the test framework. Gives the macros:
// VERIFY(condition) - aborts if not true
// CHECK(condition)
// CHECK_TITLE(condition, title)
// CHECK_EQ(expr, val)
// CHECK_NEQ(expr, val)
// CHECK_EQ_TITLE(expr, val, title)
// CHECK_NEQ_TITLE(expr, val, title)
// CHECK_ERROR(expr, exception)
// BEGIN_TEST(name)
// END_TEST
// And the types:
// TestResult
// Test

class TestResult {
public:
	TestResult() : total(0), failed(0), crashed(0), aborted(false) {}

	nat total;
	nat failed;
	nat crashed;
	bool aborted;

	inline nat passed() const {
		return total - failed - crashed;
	}

	inline TestResult &operator +=(const TestResult &other) {
		total += other.total;
		failed += other.failed;
		crashed += other.crashed;
		aborted |= other.aborted;
		return *this;
	}

	inline bool ok() const {
		return failed == 0 && crashed == 0 && !aborted;
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
	if (r.failed == 1)
		to << L"\nFailed 1 test!";
	else if (r.failed > 0)
		to << L"\nFailed " << r.failed << L" tests!";
	if (r.crashed == 1)
		to << L"\nCrashed 1 test!";
	else if (r.crashed > 0)
		to << L"\nCrashed " << r.crashed << L" tests!";
	if (r.aborted)
		to << L"\nABORTED";
	return to;
}

class Test;
class Suite;

class Tests : Singleton {
public:
	inline Tests() : singleSuite(null), singleTest(null) {}

	// Run all tests, returns the statistics.
	static TestResult run(int argc, const wchar_t *const *argv);

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

	void runTests(TestResult &result, bool runAll);
	void runSuite(Suite *s, TestResult &result, bool runAll);
	int countSuite(Suite *s);
};

// Base class for all test suites.
class Suite : NoCopy {
public:
	const String name;
	const int order;
	const bool single;
	const bool ignore;

protected:
	Suite(const String &name, int order, bool single, bool ignore, bool onDemand)
		: name(name), order(order), single(single), ignore(ignore) {
		if (!onDemand)
			Tests::instance().addSuite(this, single);
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

// Error thrown when we shall abort.
class AbortError : public ::Exception {
public:
	String what() const { return L"Aborted"; }
};

// Generic test error.
class TestError : public ::Exception {
public:
	TestError(const String &msg) : msg(msg) {}
	virtual String what() const { return msg; }
private:
	String msg;
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

template <class T, class U>
void verifyLt(TestResult &r, const T &lhs, const U &rhs, const String &expr) {
	if (!(lhs < rhs)) {
		r.failed++;
		std::wcout << L"Failed: " << expr << L" == " << lhs << L" !< " << rhs << std::endl;
	}
}

template <class T, class U>
void verifyLte(TestResult &r, const T &lhs, const U &rhs, const String &expr) {
	if (!(lhs <= rhs)) {
		r.failed++;
		std::wcout << L"Failed: " << expr << L" == " << lhs << L" !<= " << rhs << std::endl;
	}
}

template <class T, class U>
void verifyGt(TestResult &r, const T &lhs, const U &rhs, const String &expr) {
	if (!(lhs > rhs)) {
		r.failed++;
		std::wcout << L"Failed: " << expr << L" == " << lhs << L" !> " << rhs << std::endl;
	}
}

template <class T, class U>
void verifyGte(TestResult &r, const T &lhs, const U &rhs, const String &expr) {
	if (!(lhs >= rhs)) {
		r.failed++;
		std::wcout << L"Failed: " << expr << L" == " << lhs << L" !>= " << rhs << std::endl;
	}
}

#define OUTPUT_ERROR(expr, error)									  \
	std::wcout << L"Crashed " << expr << L":\n" << error << std::endl; \
	__result__.crashed++

#define DEFINE_SUITE(name, order, single, disable, demand)				\
	class Suite_##name : public Suite {									\
	private:															\
	Suite_##name() : Suite(WIDEN(#name), order, single, disable, demand) {}	\
	public:																\
	static Suite &instance() {											\
		static Suite_##name s;											\
		return s;														\
	}																	\
	}

#define EXPAND(x) x // Hack for msvc
#define PICK_FIRST(a, ...) a
#define PICK_NO_3(a, b, c, ...) c
#define NUM_ARGS(...) EXPAND(PICK_NO_3(__VA_ARGS__, 1, 0))

#define SUITE_OR_NULL_0(dummy)     null
#define SUITE_OR_NULL_1(dummy, type) &Suite_##type::instance()

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

#define CHECK_TITLE(expr, title)								\
	do {														\
		try {													\
			__result__.total++;									\
			if (!(expr)) {										\
				__result__.failed++;							\
				std::wcout << "Failed: " << title << std::endl; \
			}													\
		} catch (const storm::Exception *e) {					\
			OUTPUT_ERROR(title, e);								\
		} catch (const ::Exception &e) {						\
			OUTPUT_ERROR(title, e);								\
		} catch (...) {											\
			OUTPUT_ERROR(title, "unknown crash");				\
		}														\
	} while (false)

#define CHECK_PRED_TITLE(pred, expr, eq, title)				\
	do {													\
		try {												\
			__result__.total++;								\
			std::wostringstream __stream__;					\
			__stream__ << title;							\
			pred(__result__, expr, eq, __stream__.str());	\
		} catch (const storm::Exception *e) {				\
			OUTPUT_ERROR(title, e);							\
		} catch (const ::Exception &e) {					\
			OUTPUT_ERROR(title, e);							\
		} catch (...) {										\
			OUTPUT_ERROR(title, "unknown crash");			\
		}													\
	} while (false)

#define CHECK_EQ_TITLE(expr, eq, title)			\
	CHECK_PRED_TITLE(verifyEq, expr, eq, title)

#define CHECK_NEQ_TITLE(expr, eq, title)		\
	CHECK_PRED_TITLE(verifyNeq, expr, eq, title)

#define CHECK_LT_TITLE(expr, eq, title)			\
	CHECK_PRED_TITLE(verifyLt, expr, eq, title)

#define CHECK_LTE_TITLE(expr, eq, title)		\
	CHECK_PRED_TITLE(verifyLte, expr, eq, title)

#define CHECK_GT_TITLE(expr, eq, title)			\
	CHECK_PRED_TITLE(verifyGt, expr, eq, title)

#define CHECK_GTE_TITLE(expr, eq, title)		\
	CHECK_PRED_TITLE(verifyGte, expr, eq, title)

#define CHECK(expr) CHECK_TITLE(expr, #expr)

#define CHECK_EQ(expr, eq) CHECK_EQ_TITLE(expr, eq, #expr)

#define CHECK_NEQ(expr, eq) CHECK_NEQ_TITLE(expr, eq, #expr)

#define CHECK_LT(expr, eq) CHECK_LT_TITLE(expr, eq, #expr)

#define CHECK_LTE(expr, eq) CHECK_LTE_TITLE(expr, eq, #expr)

#define CHECK_GT(expr, eq) CHECK_GT_TITLE(expr, eq, #expr)

#define CHECK_GTE(expr, eq) CHECK_GTE_TITLE(expr, eq, #expr)

#define CHECK_ERROR(expr, type)											\
	do {																\
		try {															\
			__result__.total++;											\
			expr;														\
			__result__.failed++;										\
			std::wcout << L"Failed: " << #expr << L", did not fail as expected" << std::endl; \
		} catch (const type &) {										\
		} catch (const type *) {										\
		} catch (const storm::Exception *e) {							\
			std::wcout << L"Failed: " << #expr << L", did not throw " << #type << L" as expected." << std::endl; \
			std::wcout << e << std::endl;								\
			__result__.failed++;										\
		} catch (const ::Exception &e) {								\
			std::wcout << L"Failed: " << #expr << L", did not throw " << #type << L" as expected." << std::endl; \
			std::wcout << e << std::endl;								\
			__result__.failed++;										\
		} catch (...) {													\
			OUTPUT_ERROR(#expr, "unknown error");						\
		}																\
	} while (false)

#define CHECK_RUNS(expr)							\
	do {											\
		try {										\
			__result__.total++;						\
			expr;									\
		} catch (const storm::Exception *e) {		\
			OUTPUT_ERROR(#expr, e);					\
		} catch (const ::Exception &e) {			\
			OUTPUT_ERROR(#expr, e);					\
		} catch (...) {								\
			OUTPUT_ERROR(#expr, "unknown crash");	\
		}											\
	} while (false)

#define VERIFY(expr)													\
	do {																\
		__result__.total++;												\
		if (!(expr)) {													\
			__result__.crashed++;										\
			std::wcout << L"Failed: " << #expr << L", aborting...\n";	\
			throw AbortError();											\
		}																\
	} while (false)

#define SUITE(name, order)						\
	DEFINE_SUITE(name, order, false, false, false)

#define SUITE_(name, order)						\
	DEFINE_SUITE(name, order, true, false, false)

#define SUITEX(name, order)						\
	DEFINE_SUITE(name, order, false, true, false)

#define SUITED(name, order)						\
	DEFINE_SUITE(name, order, false, true, true)

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
