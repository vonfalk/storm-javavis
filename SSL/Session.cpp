#include "stdafx.h"
#include "Session.h"

#include "WinSChannel.h"

namespace ssl {

	// TODO: Platform independence...

	Session::Session(IStream *input, OStream *output, SSLSession *ctx) : src(input), dst(output), data(ctx) {
		SChannelSession *session = (SChannelSession *)ctx;

		// Fair initial size. We reuse the buffer as much as we can.
		Buffer buffer = storm::buffer(engine(), 1*1024);

		while (true) {
			// TODO: We need to figure out when messages are large enough. the underlying API seems
			// to not handle that too well (or we give it wrong parameters somewhere...).

			PLN(L"In bytes: " << buffer.filled());
			int result = session->initSession(engine(), buffer);
			PLN(L"Out bytes: " << buffer.filled());
			PVAR(result);
			if (result == 0) {
				break;
			} else if (result > 0) {
				// Read more data.
				Nat more = result;
				if (buffer.free() < more)
					buffer = grow(engine(), buffer, buffer.filled() + more);

				buffer = src->read(buffer);
			} else {
				// Send to server and start receiving.
				dst->write(buffer);

				// Read back data.
				buffer.filled(0);
				buffer = src->read(buffer);
			}
		}

		PLN(L"Done!");
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
