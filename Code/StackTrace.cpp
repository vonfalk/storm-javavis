#include "stdafx.h"
#include "StackTrace.h"
#include "FnLookup.h"

#include <iomanip>

// System specific headers.
#include "X86/Seh.h"

namespace code {

	StackTrace::StackTrace() : frames(null), size(0) {}

	StackTrace::StackTrace(nat n) : frames(new StackFrame[n]), size(n) {}

	StackTrace::StackTrace(const StackTrace &o) : frames(null) {
		size = o.size;
		if (o.frames) {
			frames = new StackFrame[size];
			for (nat i = 0; i < size; i++)
				frames[i] = o.frames[i];
		}
	}

	StackTrace &StackTrace::operator =(const StackTrace &o) {
		StackTrace copy(o);
		swap(frames, copy.frames);
		swap(size, copy.size);
		return *this;
	}

	StackTrace::~StackTrace() {
		delete []frames;
	}

	void StackTrace::output(wostream &to) const {
		for (nat i = 0; i < count(); i++) {
			to << endl << std::setw(3) << i;
			to << L": " << frames[i].code;
		}
	}

	String format(const StackTrace &t, const FnLookup &lookup) {
		std::wostringstream to;

		for (nat i = 0; i < t.count(); i++)
			to << std::setw(3) << i << L": " << lookup.format(t[i]) << endl;

		return to.str();
	}

	String format(const StackTrace &t) {
		return format(t, CppLookup());
	}

	String format(const StackTrace &t, Arena &arena) {
		return format(t, ArenaLookup(arena));
	}

	/**
	 * System specific code for collecting the stack trace itself.
	 */

#if defined(X86) && defined(WINDOWS)

	static bool onStack(void *stackMin, void *stackMax, void *ptr) {
		return ptr >= stackMin
			&& ptr <= stackMax;
	}

	static void *prevFrame(void *bp) {
		void **v = (void **)bp;
		return v[0];
	}

	static void *prevIp(void *bp) {
		void **v = (void **)bp;
		return v[1];
	}

	static void *prevParam(void *bp, nat id) {
		void **v = (void **)bp;
		return v[2 + id];
	}

	StackTrace stackTrace() {
		NT_TIB *tib = machineX86::getTIB();
		void *stackMax = tib->StackBase;
		void *stackMin = tib->StackLimit;

		void *base = null;
		__asm {
			mov base, ebp;
		}

		// Count frames.
		nat frames = 0;
		for (void *now = base; onStack(stackMin, stackMax, prevFrame(now)); now = prevFrame(now))
			frames++;

		// Collect the trace itself.
		StackTrace r(frames);
		void *now = base;
		for (nat i = 0; i < frames; i++) {
			StackFrame &f = r[i];
			f.code = prevIp(now);
			for (nat j = 0; j < StackFrame::maxParams; j++)
				f.params[j] = prevParam(now, j);
			now = prevFrame(now);
		}

		return r;
	}

#endif

}
