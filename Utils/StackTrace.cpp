#include "stdafx.h"
#include "StackTrace.h"
#include "FnLookup.h"

#include <iomanip>

// System specific headers.
#include "DbgHelper.h"

StackTrace::StackTrace() : frames(null), size(0), capacity(0) {}

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
	swap(capacity, copy.capacity);
	return *this;
}

StackTrace::~StackTrace() {
	delete []frames;
}

void StackTrace::push(const StackFrame &frame) {
	if (size >= capacity) {
		if (capacity == 0)
			capacity = 8;
		else
			capacity *= 2;

		StackFrame *n = new StackFrame[capacity];

		if (frames)
			for (nat i = 0; i < size; i++)
				n[i] = frames[i];

		swap(n, frames);
		delete []n;
	}

	frames[size++] = frame;
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

String cppFnName(const void *ptr) {
	StackFrame f;
	f.code = ptr;
	return CppLookup().format(f);
}

/**
 * System specific code for collecting the stack trace itself.
 */

// Windows stack traces using DbgHelp. Relies on debug information.
#if !defined(STANDALONE_STACKWALK) && defined(WINDOWS)

#if defined(X86)
static const DWORD machineType = IMAGE_FILE_MACHINE_I386;

static void initFrame(CONTEXT &context, STACKFRAME64 &frame) {
	frame.AddrPC.Offset = context.Eip;
	frame.AddrPC.Mode = AddrModeFlat;
	frame.AddrFrame.Offset = context.Ebp;
	frame.AddrFrame.Mode = AddrModeFlat;
	frame.AddrStack.Offset = context.Esp;
	frame.AddrStack.Mode = AddrModeFlat;
}

#elif defined(X64)
static const DWORD machineType = IMAGE_FILE_MACHINE_AMD64;

static void initFrame(CONTEXT &context, STACKFRAME64 &frame) {
	frame.AddrPC.Offset = context.Rip;
	frame.AddrPC.Mode = AddrModeFlat;
	frame.AddrFrame.Offset = context.Rsp; // is this correct?
	frame.AddrFrame.Mode = AddrModeFlat;
	frame.AddrStack.Offset = context.Rsp;
	frame.AddrStack.Mode = AddrModeFlat;
}
#else
#error "Unknown windows platform!"
#endif

// Warning about not being able to protect from stack-overruns...
#pragma warning ( disable : 4748 )

StackTrace stackTrace(nat skip) {
	// Initialize the library if it is not already done.
	dbgHelp();

	CONTEXT context;

#ifdef X64
	RtlCaptureContext(&context);
#else
	// Sometimes RtlCaptureContext crashes for X86, so we do it with inline-assembly instead!
	__asm {
	label:
		mov [context.Ebp], ebp;
		mov [context.Esp], esp;
		mov eax, [label];
		mov [context.Eip], eax;
	}
#endif

	HANDLE process = GetCurrentProcess();
	HANDLE thread = GetCurrentThread();
	STACKFRAME64 frame;
	zeroMem(frame);
	initFrame(context, frame);

	StackTrace r;

	while (StackWalk64(machineType, process, thread, &frame, &context, NULL, NULL, NULL, NULL)) {
		if (skip > 0) {
			skip--;
			continue;
		}

		StackFrame f;

		f.code = (void *)frame.AddrPC.Offset;
		// parameters...

		r.push(f);
	}

	return r;
}

#endif

// The stand-alone stack walk for X86 windows. Fails when frame pointer is omitted.
#if defined(STANDALONE_STACKWALK) && defined(X86) && defined(WINDOWS)

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

static NT_TIB *getTIB() {
	NT_TIB *tib;
	__asm {
		// read 'self' from 'fs:0x18'
		mov eax, fs:0x18;
		mov tib, eax;
	}
	assert(tib == tib->Self);
	return tib;
}

StackTrace stackTrace(nat skip) {
	NT_TIB *tib = getTIB();
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

	if (frames > skip)
		frames -= skip;
	else
		skip = 0;

	// Collect the trace itself.
	StackTrace r(frames);
	void *now = base;
	for (nat i = 0; i < skip; i++)
		now = prevFrame(now);

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

#ifdef POSIX

StackTrace stackTrace(nat skip) {
	// Note: We can probably get a decent stack trace by using the _Unwind_Backtrace()-function in libgcc.
	return StackTrace();
}

#endif

void dumpStack() {
	StackTrace s = stackTrace();
	PLN(format(s));
}
