#pragma once

#include "Test.h"

// The test manager. Only to be included from one cpp-file per project.


Tests &Tests::instance() {
	static Tests t;
	return t;
}

void Tests::addTest(Test *t, bool single) {
	instance().tests.insert(make_pair(t->name, t));
	if (single)
		instance().singleTest = true;
}

void Tests::addSuite(Suite *s, bool single) {
	instance().suites.insert(make_pair(s->order, s));
	if (single)
		instance().singleSuite = true;
}

void Tests::runSuite(Suite *s, TestResult &r) {
	for (TestMap::const_iterator i = tests.begin(); i != tests.end(); i++) {
		if (singleTest && !i->second->single)
			continue;

		if (i->second->suite == s) {
			std::wcout << L"Running " << i->first << L"..." << std::endl;
			try {
				r += i->second->run();
			} catch (const AbortError &) {
				std::wcout << L"Aborted..." << std::endl;
			}
		}
	}
}

int Tests::countSuite(Suite *s) {
	int r = 0;
	for (TestMap::const_iterator i = tests.begin(); i != tests.end(); i++) {
		if (i->second->suite == s) {
			if (!singleTest || i->second->single) {
				r++;
			}
		}
	}
	return r;
}

void Tests::runTests(TestResult &r) {
	if (singleTest) {
		for (TestMap::const_iterator i = tests.begin(); i != tests.end(); i++) {
			if (i->second->single) {
				std::wcout << L"Running " << i->first << L"..." << std::endl;
				r += i->second->run();
			}
		}
	} else {
		for (SuiteMap::const_iterator i = suites.begin(); i != suites.end(); i++) {
			if (singleSuite && !i->second->single)
				continue;

			std::wcout << L"--- " << i->second->name << L" ---" << std::endl;
			runSuite(i->second, r);
		}

		// Run any rogue tests not in any suite...
		if (!singleSuite && countSuite(null) > 0) {
			std::wcout << L"--- <no suite> ---" << std::endl;
			runSuite(null, r);
		}
	}
}

TestResult Tests::run() {
	Tests &t = instance();
	TestResult r;

	try {
		t.runTests(r);

		std::wcout << L"--- Results ---" << endl;
		std::wcout << r << std::endl;

	} catch (const Exception &e) {
		PLN(L"Error while testing: " << e);
		PLN(L"ABORTED");
	}

	return r;
}
