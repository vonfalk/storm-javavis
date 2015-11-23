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

}
