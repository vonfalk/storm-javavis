#include "stdafx.h"
#include "NamedThread.h"
#include "Lib/Str.h"

namespace storm {

	NamedThread::NamedThread(const String &name) : Named(name), pos() {
		thread = CREATE(Thread, this);
	}

	NamedThread::NamedThread(const String &name, Par<Thread> t) : Named(name), pos(), thread(t) {}

	NamedThread::NamedThread(Par<Str> name) : Named(name->v), pos() {
		thread = CREATE(Thread, this);
	}

	NamedThread::NamedThread(Par<SStr> name) : Named(name->v->v), pos(name->pos) {
		thread = CREATE(Thread, this);
	}

	void NamedThread::output(wostream &to) const {
		to << L"thread " << identifier();
	}

}
