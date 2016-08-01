#include "stdafx.h"
#include "NamedThread.h"
#include "Core/Thread.h"
#include "Core/StrBuf.h"

namespace storm {

	NamedThread::NamedThread(Str *name) : Named(name) {
		myThread = new (this) Thread();
	}

	NamedThread::NamedThread(Str *name, Thread *thread) : Named(name), myThread(thread) {}

	void NamedThread::toS(StrBuf *to) const {
		*to << L"thread " << identifier();
	}

}
