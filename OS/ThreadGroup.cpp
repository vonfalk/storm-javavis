#include "stdafx.h"
#include "ThreadGroup.h"

namespace os {

	ThreadGroup::ThreadGroup() : data(new ThreadGroupData()) {}

	ThreadGroup::ThreadGroup(Callback start, Callback end) : data(new ThreadGroupData(start, end)) {}

	ThreadGroup::ThreadGroup(const ThreadGroup &o) : data(o.data) {
		data->addRef();
	}

	ThreadGroup::~ThreadGroup() {
		data->release();
	}

	ThreadGroup &ThreadGroup::operator =(const ThreadGroup &o) {
		data->release();
		data = o.data;
		data->addRef();
		return *this;
	}

	void ThreadGroup::join() {
		data->join();
	}


	ThreadGroupData::ThreadGroupData() : references(1), attached(0), sema(0) {}

	ThreadGroupData::ThreadGroupData(ThreadGroup::Callback start, ThreadGroup::Callback stop)
		: references(1), attached(0), sema(0), start(start), stop(stop) {}

	ThreadGroupData::~ThreadGroupData() {}

	void ThreadGroupData::threadStarted() {
		atomicIncrement(attached);
		start();
	}

	void ThreadGroupData::threadTerminated() {
		stop();
		sema.up();
	}

	void ThreadGroupData::join() {
		// Wait for all threads in turn.
		while (atomicRead(attached) > 0) {
			atomicDecrement(attached);
			sema.down();
		}
	}

}
