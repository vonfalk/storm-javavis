#pragma once

#include "Test.h"

// The test manager. Only to be included from one cpp-file per project.


Tests &Tests::instance() {
	static Tests t;
	return t;
}

void Tests::addTest(Test *t) {
	instance().tests.insert(std::make_pair(t->getName(), t));
}

TestResult Tests::run() {
	Tests &t = instance();

	TestResult r;
	for (TestMap::const_iterator i = t.tests.begin(); i != t.tests.end(); i++) {
		std::wcout << L"Running " << i->first << L"..." << std::endl;
		r += i->second->run();
	}

	std::wcout << r << std::endl;

	return r;
}
