#include "stdafx.h"
#include "NamedThread.h"
#include "Engine.h"
#include "Shared/Str.h"

namespace storm {

	NamedThread::NamedThread(const String &name) : Named(name), pos() {
		init();
	}

	NamedThread::NamedThread(const String &name, Par<Thread> t) : Named(name), pos() {
		init(t.borrow());
	}

	NamedThread::NamedThread(Par<Str> name) : Named(name->v), pos() {
		init();
	}

	NamedThread::NamedThread(Par<SStr> name) : Named(name->v->v), pos(name->pos) {
		init();
	}

	void NamedThread::init(Thread *t) {
		if (t) {
			myThread = capture(t);
		} else {
			myThread = CREATE(Thread, this);
		}
		reference = null;
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

}
