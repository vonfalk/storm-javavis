#include "stdafx.h"
#include "Session.h"

#include "WinSChannel.h"

namespace ssl {

	// TODO: Platform independence...

	Session::Session(IStream *input, OStream *output, SSLSession *ctx) : src(input), dst(output), data(ctx) {
		SChannelSession *session = (SChannelSession *)ctx;

		// Minimum "free" when calling receive.
		Nat minFree = 128;

		// Input and output buffers.
		inBuffer = storm::buffer(engine(), 2*1024);
		outBuffer = storm::buffer(engine(), 2*1024);

		while (true) {
			int result = session->initSession(engine(), inBuffer, outBuffer);

			if (result == 0) {
				break;
			} else if (result < 0) {
				// Send to server and receive more.
				if (outBuffer.filled() > 0) {
					dst->write(outBuffer);
					dst->flush();
				}
			}

			// Read more data.
			Nat more = max(minFree, Nat(abs(result)));
			if (inBuffer.free() < more)
				inBuffer = grow(engine(), inBuffer, inBuffer.filled() + more);

			inBuffer = src->read(inBuffer);
		}

		// From here on, we can use EncryptMessage and DecryptMessage.
		// We must use cbMaximumMessage from QueryContextAttributes to find max message size.

		// Resize the input and output buffers so that we don't have to resize them in the
		// future. We can't reallocate them and keep inter-thread consistency.
		Nat bufferSizes = min(Nat(4*1024), session->maxBlockSize);
		inBuffer = storm::buffer(engine(), bufferSizes);
		outBuffer = storm::buffer(engine(), bufferSizes);
	}

	Session::Session(const Session &o) : src(o.src), dst(o.dst), data(o.data) {
		data->ref();
	}

	Session::~Session() {
		data->unref();
	}

	void Session::deepCopy(CloneEnv *) {
		// No need.
	}

	IStream *Session::input() {
		return new (this) SessionIStream(this);
	}

	OStream *Session::output() {
		return new (this) SessionOStream(this);
	}

	void Session::close() {
		src->close();
		dst->close();
	}


	/**
	 * Input stream.
	 */

	SessionIStream::SessionIStream(Session *owner) : owner(owner) {}

	void SessionIStream::close() {}

	Bool SessionIStream::more() {
		return true;
	}

	Buffer SessionIStream::read(Buffer to) {
		return to;
	}

	Buffer SessionIStream::peek(Buffer to) {
		return to;
	}


	/**
	 * Output stream.
	 */

	SessionOStream::SessionOStream(Session *owner) : owner(owner) {}

	void SessionOStream::close() {}

	void SessionOStream::write(Buffer from, Nat offset) {
		PLN(TO_S(this, from));
	}

	void SessionOStream::flush() {}

}
