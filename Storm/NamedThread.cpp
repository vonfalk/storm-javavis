#include "stdafx.h"
#include "NamedThread.h"
#include "Engine.h"
#include "Shared/Str.h"

namespace storm {

	NamedThread::NamedThread(const String &name) : Named(name), pos(), reference(null) {
		myThread = CREATE(Thread, this);
	}

	NamedThread::NamedThread(const String &name, Par<Thread> t) : Named(name), pos(), myThread(t), reference(null) {}

	NamedThread::NamedThread(Par<Str> name) : Named(name->v), pos(), reference(null) {
		myThread = CREATE(Thread, this);
	}

	NamedThread::NamedThread(Par<SStr> name) : Named(name->v->v), pos(name->pos), reference(null) {
		myThread = CREATE(Thread, this);
	}

	NamedThread::~NamedThread() {
		delete reference;
	}

	code::Ref NamedThread::ref() {
		if (!reference) {
			reference = new code::RefSource(engine().arena, identifier());
			reference->setPtr(myThread.borrow());
		}
		return *reference;
	}

	void NamedThread::output(wostream &to) const {
		to << L"thread " << identifier();
	}

	RunOn::RunOn(State state) : state(state) {}

	RunOn::RunOn(Par<NamedThread> t) : state(named), thread(t) {}

	Bool RunOn::canRun(const RunOn &o) const {
		// Anyone can run code declared as 'any'.
		if (o.state == any)
			return true;

		// If the other one is declared as 'runtime', we can not know.
		if (o.state == runtime)
			return false;

		if (state != o.state)
			return false;
		if (state == named && thread.borrow() != o.thread.borrow())
			return false;

		return true;
	}

	RunOn runOn(Par<NamedThread> thread) {
		return RunOn(thread);
	}

	RunOn runOnAny() {
		return RunOn(RunOn::any);
	}

	RunOn runOnRuntime() {
		return RunOn(RunOn::runtime);
	}


	wostream &operator <<(wostream &to, const RunOn &v) {
		switch (v.state) {
		case RunOn::any:
			to << L"any";
			break;
		case RunOn::runtime:
			to << L"runtime";
			break;
		case RunOn::named:
			to << L"named: " << v.thread->identifier();
			break;
		}
		return to;
	}
}
