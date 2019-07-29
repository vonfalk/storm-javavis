#include "stdafx.h"
#include "ThreadGroup.h"
#include "Thread.h"

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

	vector<Thread> ThreadGroup::threads() const {
		return data->threads();
	}

	void ThreadGroup::join() {
		data->join();
	}


	ThreadGroupData::ThreadGroupData() : references(1), attached(0), sema(0) {}

	ThreadGroupData::ThreadGroupData(ThreadGroup::Callback start, ThreadGroup::Callback stop)
		: references(1), attached(0), sema(0), start(start), stop(stop) {}

	ThreadGroupData::~ThreadGroupData() {}

	void ThreadGroupData::threadStarted(ThreadData *data) {
		atomicIncrement(attached);
		start();

		util::Lock::L z(runningLock);
		running.insert(data);
	}

	bool ThreadGroupData::threadUnreachable(ThreadData *data) {
		util::Lock::L z(runningLock);

		// We handed out another reference. Abort!
		if (atomicRead(data->references) > 0)
			return false;

		running.erase(data);
		return true;
	}

	void ThreadGroupData::threadTerminated() {
		stop();
		sema.up();

		// TODO: Perhaps we shall decrease 'attached' at some time to avoid overflow?
	}

	vector<Thread> ThreadGroupData::threads() {
		vector<Thread> result;

		util::Lock::L z(runningLock);
		for (InlineSet<ThreadData>::iterator i = running.begin(); i != running.end(); ++i) {
			result.push_back(Thread(i));
		}

		return result;
	}

	void ThreadGroupData::join() {
		// Wait for all threads in turn.
		while (atomicRead(attached) > 0) {
			atomicDecrement(attached);
			sema.down();
		}
	}

}
