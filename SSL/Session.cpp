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
		Buffer inBuffer = storm::buffer(engine(), 4*1024);
		Buffer outBuffer;

		while (true) {
			int result = session->initSession(engine(), inBuffer, outBuffer);

			if (result == 0) {
				break;
			} else if (result < 0) {
				// Send to server and receive more.
				if (outBuffer.filled() > 0)
					dst->write(outBuffer);
			}

			// Read more data.
			Nat more = max(minFree, Nat(abs(result)));
			if (inBuffer.free() < more)
				inBuffer = grow(engine(), inBuffer, inBuffer.filled() + more);

			inBuffer = src->read(inBuffer);
		}

		PVAR(session->maxBlockSize);

		// From here on, we can use EncryptMessage and DecryptMessage.
		// We must use cbMaximumMessage from QueryContextAttributes to find max message size.
	}

	Session::Session(const Session &o) : src(o.src), dst(o.dst), data(o.data) {
		data->ref();
	}

	Session::~Session() {
		data->unref();
	}

	void Session::deepCopy(CloneEnv *) {
		TODO(L"Think about how to handle copies.");
	}

	void Session::close() {
		src->close();
		dst->close();
	}

}
