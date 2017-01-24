#include "stdafx.h"
#include "HandleStream.h"
#include "OS/IORequest.h"

namespace storm {

	/**
	 * System-specific helpers. These all behave as if the handle was blocking.
	 */

#ifdef WINDOWS

	static inline void close(os::Handle &h) {
		CloseHandle(h.v());
		h = os::Handle();
	}

	static Nat read(os::Handle h, os::Thread &attached, void *dest, Nat limit) {
		if (attached == os::Thread::invalid) {
			attached = os::Thread::current();
			attached.attach(h);
		}

		os::IORequest request;

		LARGE_INTEGER pos;
		pos.QuadPart = 0;
		if (SetFilePointerEx(h.v(), pos, &pos, FILE_CURRENT)) {
			// There seems to be a poblem when reading from the end of a file asynchronously.
			LARGE_INTEGER len;
			GetFileSizeEx(h.v(), &len);
			if (pos.QuadPart >= len.QuadPart)
				return 0;

			request.Offset = pos.LowPart;
			request.OffsetHigh = pos.HighPart;
		} else {
			// If we can not seek, it means that the offset should be zero.
			pos.QuadPart = 0;
		}

		if (ReadFile(h.v(), dest, DWORD(limit), NULL, &request)) {
			// Completed synchronusly.
		} else if (GetLastError() == ERROR_IO_PENDING) {
			// Completing async...
			request.wake.down();

			// Advance the file pointer.
			pos.QuadPart = request.bytes;
			SetFilePointerEx(h.v(), pos, NULL, FILE_CURRENT);
		} else {
			// Failed.
			request.bytes = 0;
		}

		return request.bytes;
	}

	static Nat write(os::Handle h, os::Thread &attached, const void *src, Nat limit) {
		if (attached == os::Thread::invalid) {
			attached = os::Thread::current();
			attached.attach(h);
		}

		os::IORequest request;

		LARGE_INTEGER pos;
		pos.QuadPart = 0;
		if (SetFilePointerEx(h.v(), pos, &pos, FILE_CURRENT)) {
			// All is well.
			request.Offset = pos.LowPart;
			request.OffsetHigh = pos.HighPart;
		} else {
			// If we can not seek, it means that the offset should be zero.
			pos.QuadPart = 0;
		}

		if (WriteFile(h.v(), src, DWORD(limit), NULL, &request)) {
			// Completed synchronusly.
		} else if (GetLastError() == ERROR_IO_PENDING) {
			// Completing async...
			request.wake.down();

			// Advance the file pointer.
			pos.QuadPart = request.bytes;
			SetFilePointerEx(h.v(), pos, NULL, FILE_CURRENT);
		} else {
			// Failed.
			request.bytes = 0;
		}

		return request.bytes;
	}

	static void seek(os::Handle h, Word to) {
		LARGE_INTEGER pos;
		pos.QuadPart = to;
		SetFilePointerEx(h.v(), pos, NULL, FILE_BEGIN);
	}

	static Word tell(os::Handle h) {
		LARGE_INTEGER pos;
		pos.QuadPart = 0;
		SetFilePointerEx(h.v(), pos, &pos, FILE_CURRENT);
		return pos.QuadPart;
	}

	static Word length(os::Handle h) {
		LARGE_INTEGER len;
		GetFileSizeEx(h.v(), &len);
		return len.QuadPart;
	}

	static os::Handle openStd(DWORD id, bool input) {
		return GetStdHandle(id);
	}

	namespace proc {

		IStream *in(EnginePtr e) {
			return new (e.v) OSIStream(openStd(STD_INPUT_HANDLE, true));
		}

		OStream *out(EnginePtr e) {
			return new (e.v) OSOStream(openStd(STD_OUTPUT_HANDLE, false));
		}

		OStream *error(EnginePtr e) {
			return new (e.v) OSOStream(openStd(STD_ERROR_HANDLE, false));
		}

	}

#else
#error "Please implement me for UNIX."
#endif

	/**
	 * Regular input stream.
	 */

	static const GcType bufType = {
		GcType::tArray,
		null,
		null,
		sizeof(byte),
		0,
		{}
	};

	// Copy a buffer.
	static GcArray<byte> *copy(Engine &e, GcArray<byte> *src) {
		if (!src)
			return null;

		GcArray<byte> *dest = runtime::allocArray<byte>(e, &bufType, src->count);
		dest->filled = src->filled;
		memcpy(dest->v, src->v, src->filled);
		return dest;
	}

	OSIStream::OSIStream(os::Handle h)
		: handle(h),
		  attachedTo(os::Thread::invalid),
		  lookahead(null),
		  lookaheadStart(0),
		  atEof(false) {}

	OSIStream::OSIStream(const OSIStream &o) : handle(o.handle), attachedTo(o.attachedTo), atEof(false) {
		// TODO: If 'lookaheadStart != 0' we do not need to copy all of 'lookahead'.
		lookaheadStart = o.lookaheadStart;
		lookahead = copy(engine(), lookahead);
	}

	OSIStream::~OSIStream() {
		if (handle)
			storm::close(handle);
	}

	void OSIStream::deepCopy(CloneEnv *e) {
		// Nothing needs to be done here.
	}

	Bool OSIStream::more() {
		if (!handle)
			return false;

		return !atEof;
	}

	Buffer OSIStream::read(Buffer b, Nat start) {
		start = min(start, b.count());
		b.filled(0);

		Nat read = b.count() - start;

		// Is there anything left in the lookahead for us to consume?
		Nat avail = lookaheadAvail();
		if (avail > 0) {
			Nat copy = min(read, avail);
			memcpy(b.dataPtr() + start, lookahead->v + lookaheadStart, copy);
			lookaheadStart += copy;
			start += copy;
			read -= copy;
			b.filled(start);
		}

		// Done reading from the lookahead alone?
		if (read == 0)
			return b;

		if (!handle)
			return b;

		Nat r = storm::read(handle, attachedTo, b.dataPtr() + start, read);
		if (r == 0)
			atEof = true;
		b.filled(r + start);
		return b;
	}

	Buffer OSIStream::peek(Buffer b, Nat start) {
		start = min(start, b.count());
		b.filled(0);
		if (!handle)
			return b;

		Nat toPeek = b.count() - start;

		Nat avail = doLookahead(toPeek);
		if (!lookahead)
			return b;

		toPeek = min(toPeek, avail);
		memcpy(b.dataPtr() + start, lookahead->v + lookaheadStart, toPeek);
		b.filled(toPeek + start);

		return b;
	}

	Nat OSIStream::lookaheadAvail() {
		if (!lookahead)
			return 0;

		return lookahead->filled - lookaheadStart;
	}

	Nat OSIStream::doLookahead(Nat bytes) {
		Nat avail = lookaheadAvail();
		if (avail >= bytes)
			return avail;

		ensureLookahead(bytes);

		// We need to read more data!
		Nat read = bytes - avail;
		Nat more = storm::read(handle, attachedTo, lookahead->v + lookahead->filled, read);
		lookahead->filled += more;

		return lookaheadAvail();
	}

	void OSIStream::ensureLookahead(Nat n) {
		if (!lookahead) {
			// Easy, just allocate a new buffer.
			lookahead = runtime::allocArray<byte>(engine(), &bufType, n);
			return;
		}

		if (lookahead->count - lookaheadStart >= n) {
			// Enough space!
			return;
		}

		// We need to re-allocate...
		Nat preserve = lookahead->filled - lookaheadStart;
		GcArray<byte> *t = runtime::allocArray<byte>(engine(), &bufType, n);
		t->filled = preserve;
		memcpy(t->v, lookahead->v + lookaheadStart, preserve);
		lookahead = t;
		lookaheadStart = 0;
	}

	void OSIStream::close() {
		if (handle)
			storm::close(handle);
	}

	/**
	 * Random access stream.
	 */

	OSRIStream::OSRIStream(os::Handle h) : handle(h), attachedTo(os::Thread::invalid) {}

	OSRIStream::OSRIStream(const OSRIStream &o) : handle(o.handle), attachedTo(o.attachedTo) {}

	OSRIStream::~OSRIStream() {
		if (handle)
			storm::close(handle);
	}

	void OSRIStream::deepCopy(CloneEnv *e) {
		// Nothing needs to be done.
	}

	Bool OSRIStream::more() {
		if (!handle)
			return false;

		return tell() < length();
	}

	Buffer OSRIStream::read(Buffer b, Nat start) {
		start = min(start, b.count());
		b.filled(0);

		if (!handle)
			return b;

		Nat r = storm::read(handle, attachedTo, b.dataPtr() + start, b.count() - start);
		b.filled(r + start);
		return b;
	}

	Buffer OSRIStream::peek(Buffer b, Nat start) {
		b.filled(0);

		if (!handle)
			return b;

		Word pos = tell();
		b = read(b, start);
		seek(pos);

		return b;
	}

	void OSRIStream::close() {
		if (handle)
			storm::close(handle);
	}

	RIStream *OSRIStream::randomAccess() {
		return this;
	}

	void OSRIStream::seek(Word to) {
		storm::seek(handle, to);
	}

	Word OSRIStream::tell() {
		if (!handle)
			return 0;

		return storm::tell(handle);
	}

	Word OSRIStream::length() {
		if (!handle)
			return 0;

		return storm::length(handle);
	}

	/**
	 * Output stream.
	 */

	OSOStream::OSOStream(os::Handle h) : handle(h), attachedTo(os::Thread::invalid) {}

	OSOStream::OSOStream(const OSOStream &o) : handle(o.handle), attachedTo(o.attachedTo) {}

	OSOStream::~OSOStream() {
		if (handle)
			storm::close(handle);
	}

	void OSOStream::write(Buffer to, Nat start) {
		start = min(start, to.count());
		if (handle)
			storm::write(handle, attachedTo, to.dataPtr() + start, to.count() - start);
	}

	void OSOStream::close() {
		if (handle)
			storm::close(handle);
	}

}
