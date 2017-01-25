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

	static os::Handle dupHandle(os::Handle src) {
		if (!src)
			return os::Handle();

		HANDLE dest = INVALID_HANDLE_VALUE;
		HANDLE proc = GetCurrentProcess();
		if (!DuplicateHandle(proc, src.v(), proc, &dest, DUPLICATE_SAME_ACCESS, FALSE, 0)) {
			PLN(L"Failed to duplicate handle :(");
			return os::Handle();
		}

		return dest;
	}

	static os::Handle openStd(DWORD id, bool input) {
		return dupHandle(GetStdHandle(id));
	}

#else
#error "Please implement me for UNIX."
#endif

	/**
	 * Regular input stream.
	 */

	HandleIStream::HandleIStream(os::Handle h)
		: handle(h),
		  attachedTo(os::Thread::invalid) {}

	HandleIStream::HandleIStream(const HandleIStream &o)
		: PeekIStream(o),
		  handle(dupHandle(o.handle)),
		  attachedTo(os::Thread::invalid) {}

	HandleIStream::~HandleIStream() {
		if (handle)
			storm::close(handle);
	}

	Bool HandleIStream::more() {
		if (!handle)
			return false;

		return PeekIStream::more();
	}

	void HandleIStream::close() {
		if (handle)
			storm::close(handle);

		PeekIStream::close();
	}

	Nat HandleIStream::doRead(byte *to, Nat count) {
		if (handle)
			return storm::read(handle, attachedTo, to, count);
		else
			return 0;
	}

	/**
	 * Random access stream.
	 */

	HandleRIStream::HandleRIStream(os::Handle h)
		: handle(h),
		  attachedTo(os::Thread::invalid) {}

	HandleRIStream::HandleRIStream(const HandleRIStream &o)
		: handle(dupHandle(o.handle)),
		  attachedTo(os::Thread::invalid) {}

	HandleRIStream::~HandleRIStream() {
		if (handle)
			storm::close(handle);
	}

	void HandleRIStream::deepCopy(CloneEnv *e) {
		// Nothing needs to be done.
	}

	Bool HandleRIStream::more() {
		if (!handle)
			return false;

		return tell() < length();
	}

	Buffer HandleRIStream::read(Buffer b, Nat start) {
		start = min(start, b.count());
		b.filled(0);

		if (!handle)
			return b;

		Nat r = storm::read(handle, attachedTo, b.dataPtr() + start, b.count() - start);
		b.filled(r + start);
		return b;
	}

	Buffer HandleRIStream::peek(Buffer b, Nat start) {
		b.filled(0);

		if (!handle)
			return b;

		Word pos = tell();
		b = read(b, start);
		seek(pos);

		return b;
	}

	void HandleRIStream::close() {
		if (handle)
			storm::close(handle);
	}

	RIStream *HandleRIStream::randomAccess() {
		return this;
	}

	void HandleRIStream::seek(Word to) {
		storm::seek(handle, to);
	}

	Word HandleRIStream::tell() {
		if (!handle)
			return 0;

		return storm::tell(handle);
	}

	Word HandleRIStream::length() {
		if (!handle)
			return 0;

		return storm::length(handle);
	}

	/**
	 * Output stream.
	 */

	HandleOStream::HandleOStream(os::Handle h)
		: handle(h),
		  attachedTo(os::Thread::invalid) {}

	HandleOStream::HandleOStream(const HandleOStream &o)
		: handle(dupHandle(o.handle)),
		  attachedTo(os::Thread::invalid) {}

	HandleOStream::~HandleOStream() {
		if (handle)
			storm::close(handle);
	}

	void HandleOStream::write(Buffer to, Nat start) {
		start = min(start, to.filled());
		if (handle)
			storm::write(handle, attachedTo, to.dataPtr() + start, to.filled() - start);
	}

	void HandleOStream::close() {
		if (handle)
			storm::close(handle);
	}

}
