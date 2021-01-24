#include "stdafx.h"
#include "Session.h"

#include "SecureChannel.h"
#include "OpenSSL.h"

namespace ssl {

	// TODO: Platform independence...

	Session::Session(IStream *input, OStream *output, SSLSession *ctx, Str *host)
		: src(input), dst(output), data(ctx),
		  decryptedStart(0), decryptedEnd(0), remainingStart(0),
		  incomingEnd(false), outgoingEnd(false) {

#ifdef WINDOWS
		SChannelSession *session = (SChannelSession *)ctx;

		// Minimum "free" when calling receive.
		Nat minFree = 128;

		// Input and output buffers.
		Buffer inBuffer = storm::buffer(engine(), 2*1024);
		Buffer outBuffer = storm::buffer(engine(), 2*1024);

		while (true) {
			int result = session->initSession(engine(), inBuffer, outBuffer, host->c_str());

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
		bufferSizes = min(Nat(4*1024), session->maxMessageSize);
		incoming = storm::buffer(engine(), bufferSizes);
#else
		OpenSSLSession *session = (OpenSSLSession *)ctx;

		implData = session->create(input, output, host->utf8_str());
#endif
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
		shutdown();
		src->close();
		dst->close();
	}

	Bool Session::more() {
#ifdef WINDOWS
		os::Lock::L z(data->lock);
		// More decrypted data?
		if (decryptedStart < decryptedEnd)
			return true;

		// If the remote end is shutdown, we won't get more data.
		if (incomingEnd)
			return false;

		// Otherwise, see if we have more data to decrypt, or if there is more in the source.
		return (remainingStart < incoming.filled()) // more encrypted data?
			|| src->more(); // more from the source stream?
#endif
		return true;
	}

	void Session::read(Buffer &to) {
		os::Lock::L z(data->lock);

#ifdef WINDOWS
		if (decryptedStart >= decryptedEnd)
			fill();

		Nat toFill = min(to.free(), decryptedEnd - decryptedStart);
		memcpy(to.dataPtr() + to.filled(), incoming.dataPtr() + decryptedStart, toFill);
		to.filled(to.filled() + toFill);
		decryptedStart += toFill;
#else
		OpenSSLSession *session = (OpenSSLSession *)data;
		int bytes = BIO_read(session->connection, to.dataPtr() + to.filled(), to.free());
		if (bytes > 0) {
			to.filled(to.filled() + bytes);
		} else if (bytes == 0) {
			// We are at EOF!
		} else {
			// Error.
		}
#endif
	}

	void Session::peek(Buffer &to) {
		os::Lock::L z(data->lock);

#ifdef WINDOWS
		if (decryptedStart >= decryptedEnd)
			fill();

		Nat toFill = min(to.free(), decryptedEnd - decryptedStart);
		memcpy(to.dataPtr() + to.filled(), incoming.dataPtr() + decryptedStart, toFill);
		to.filled(to.filled() + toFill);
#else
		TODO(L"Implement peek! We can do as in PeekStream.");
#endif
	}

	void Session::write(const Buffer &from, Nat offset) {
		if (offset >= from.filled())
			return;

		os::Lock::L z(data->lock);

#ifdef WINDOWS
		if (outgoingEnd)
			return;

		// TODO: Respect the max message size!

		SChannelSession *session = (SChannelSession *)data;
		session->encrypt(engine(), from, offset, outgoing);
		dst->write(outgoing);

		// Clear the buffer if it becomes too large.
		if (outgoing.count() > bufferSizes * 2)
			outgoing = Buffer();
#else
		OpenSSLSession *session = (OpenSSLSession *)data;
		BIO_write(session->connection, from.dataPtr() + offset, from.filled() - offset);
#endif
	}

	void Session::flush() {
		os::Lock::L z(data->lock);
#ifdef WINDOWS
		// We don't really buffer data, so just propagate the flush.
		dst->flush();
#else
		OpenSSLSession *session = (OpenSSLSession *)data;
		BIO_flush(session->connection);
#endif
	}

	void Session::fill() {
#ifdef WINDOWS
		// In case we fail somewhere...
		decryptedStart = decryptedEnd = 0;

		// Anything left in the buffer?
		if (remainingStart >= incoming.filled()) {
			// Nope. Get more. But this means that the buffer is entirely empty, use that.
			incoming.filled(0);
			remainingStart = 0;
			incoming = src->read(incoming);
		}

		// Should we shrink the buffer a bit?
		Nat used = incoming.filled() - remainingStart;
		if ((used * 2 < incoming.count()) && (incoming.count() > bufferSizes)) {
			Buffer t = buffer(engine(), max(used, bufferSizes));
			memcpy(t.dataPtr(), incoming.dataPtr() + remainingStart, used);
			incoming = t;
			incoming.filled(used);
			remainingStart = 0;
		}

		SChannelSession *session = (SChannelSession *)data;

		while (true) {
			SChannelSession::Markers markers = session->decrypt(engine(), incoming, remainingStart);
			decryptedStart = markers.plaintextStart;
			decryptedEnd = markers.plaintextEnd;
			remainingStart = markers.remainingStart;

			// Did we decrypt something? Good, we're done for now.
			if (decryptedStart != decryptedEnd)
				break;

			// End of transmission?
			if (markers.shutdown) {
				incomingEnd = true;
				break;
			}

			// Is there a large "hole" in the beginning of the array? If so, we want to move it back
			// into place to use the space more efficiently.
			if (remainingStart > incoming.count() / 4) {
				incoming.shift(remainingStart);
				remainingStart = 0;
				decryptedStart = decryptedEnd = 0;
			}

			// If we didn't get any data back, try to get more data and try again.
			Nat margin = incoming.filled() / 2;
			if (incoming.free() < margin)
				incoming = grow(engine(), incoming, incoming.filled() * 2);

			Nat before = incoming.filled();
			incoming = src->read(incoming);

			if (incoming.filled() == before) {
				// Failed to receive data, give up.
				break;
			}
		}
#endif
	}

	void Session::shutdown() {
		os::Lock::L z(data->lock);

#ifdef WINDOWS

		if (outgoingEnd)
			return;

		SChannelSession *session = (SChannelSession *)data;
		Buffer msg = session->shutdown(engine());
		dst->write(msg);

		outgoingEnd = true;
#else
		TODO(L"Do a proper shutdown!");
#endif
	}


	/**
	 * Input stream.
	 */

	SessionIStream::SessionIStream(Session *owner) : owner(owner) {}

	void SessionIStream::close() {}

	Bool SessionIStream::more() {
		return owner->more();
	}

	Buffer SessionIStream::read(Buffer to) {
		owner->read(to);
		return to;
	}

	Buffer SessionIStream::peek(Buffer to) {
		owner->peek(to);
		return to;
	}


	/**
	 * Output stream.
	 */

	SessionOStream::SessionOStream(Session *owner) : owner(owner) {}

	void SessionOStream::close() {
		owner->shutdown();
	}

	void SessionOStream::write(Buffer from, Nat offset) {
		owner->write(from, offset);
	}

	void SessionOStream::flush() {
		owner->flush();
	}

}
