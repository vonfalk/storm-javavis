#include "stdafx.h"
#include "NamedThread.h"
#include "NamedSource.h"
#include "Core/Thread.h"
#include "Core/StrBuf.h"

namespace storm {

	NamedThread::NamedThread(SrcPos pos, Str *name) : Named(pos, name) {
		myThread = new (this) Thread();
	}

	NamedThread::NamedThread(syntax::SStr *name) : Named(name->pos, name->v) {
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

	MAYBE(Str *) NamedThread::canReplace(Named *old, ReplaceContext *) {
		if (!as<NamedThread>(old))
			return new (this) Str(S("Cannot replace anything other than a thread with a thread."));
		else
			return null;
	}

	void NamedThread::doReplace(Named *old, ReplaceTasks *tasks) {
		NamedThread *t = (NamedThread *)old;

		// Steal the references.
		myThread = t->myThread;
		reference = t->reference;

		// Make sure the new thread object is used everywhere.
		tasks->replace(old, this);
	}

}
