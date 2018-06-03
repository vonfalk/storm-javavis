#include "stdafx.h"
#include "NamedThread.h"
#include "NamedSource.h"
#include "Core/Thread.h"
#include "Core/StrBuf.h"

namespace storm {

	NamedThread::NamedThread(Str *name) : Named(name) {
		myThread = new (this) Thread();
	}

	NamedThread::NamedThread(syntax::SStr *name) : Named(name->v) {
		myThread = new (this) Thread();
	}

	NamedThread::NamedThread(Str *name, Thread *thread) : Named(name), myThread(thread) {}

	code::Ref NamedThread::ref() {
		if (!reference) {
			reference = new (this) NamedSource(this);
			reference->setPtr(myThread);
		}
		return code::Ref(reference);
	}

	void NamedThread::toS(StrBuf *to) const {
		*to << L"thread " << identifier();
	}

}
