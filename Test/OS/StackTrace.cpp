#include "stdafx.h"
#include "OS/Thread.h"
#include "OS/ThreadGroup.h"
#include "OS/StackTrace.h"
#include "OS/Sync.h"

static os::Sema waitSema(0);

static void auxThread() {
	waitSema.down();
}


BEGIN_TESTX(StackTraceTest, OS) {
	os::ThreadGroup g;

	{
		os::Thread a = os::Thread::current();
		os::Thread b = os::Thread::spawn(util::simpleVoidFn(auxThread), g);
		os::UThread::spawn(util::simpleVoidFn(auxThread));
		os::UThread::spawn(util::simpleVoidFn(auxThread), &b);

		vector<os::Thread> threads = g.threads();
		threads.push_back(a);

		vector<vector<StackTrace>> traces = os::stackTraces(threads);

		for (int i = 0; i < 4; i++)
			waitSema.up();

		for (size_t i = 0; i < traces.size(); i++) {
			PLN(L"Thread " << i << L":");
			::Indent z(util::debugStream());
			for (size_t j = 0; j < traces[i].size(); j++) {
				PLN(L"UThread " << j << L":");
				PLN(format(traces[i][j]));
			}
		}
	}

	g.join();

} END_TEST
