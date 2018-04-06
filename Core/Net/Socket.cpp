#include "stdafx.h"
#include "Socket.h"
#include "Core/CloneEnv.h"

namespace storm {


	Socket::Socket(os::Handle handle, os::Thread attachTo) : handle(handle), closed(0) {
		i = new (this) SocketIStream(this, attachTo);
		o = new (this) SocketOStream(this, attachTo);
	}

	Socket::~Socket() {
		if (handle)
			closeSocket(handle);
	}

	void Socket::close() {
		if (handle) {
			closeEnd(closeRead);
			closeEnd(closeWrite);
		}
	}

	void Socket::deepCopy(CloneEnv *env) {
		cloned(i, env);
		cloned(o, env);
	}

	void Socket::closeEnd(Nat which) {
		Nat old, w;
		do {
			old = atomicRead(closed);
			w = old | which;
		} while (atomicCAS(closed, old, w) != old);

		if (w == (closeRead | closeWrite) && handle) {
			closeSocket(handle);
			handle = os::Handle();
		}
	}

	IStream *Socket::input() const {
		return i;
	}

	OStream *Socket::output() const {
		return o;
	}

	/**
	 * Connect.
	 */

	MAYBE(Socket *) connect(Address *to) {
		initSockets();

		sockaddr_storage data;
		sockaddr *sa = (sockaddr *)&data;
		to->fill(sa);

		os::Handle h = createTcpSocket(sa->sa_family);
		os::Thread current = os::Thread::current();
		current.attach(h);

		try {
			// Connect!
			bool ok = connectSocket(h, current, sa);
			if (!ok)
				return false;

			return new (to) Socket(h, current);
		} catch (...) {
			closeSocket(h);
			throw;
		}
	}


	/**
	 * IStream.
	 */

	SocketIStream::SocketIStream(Socket *owner, os::Thread t) : HandleIStream(owner->handle, t) {}

	SocketIStream::~SocketIStream() {
		// The socket will close the handle.
		handle = os::Handle();
	}

	void SocketIStream::close() {
		owner->closeEnd(Socket::closeRead);
		handle = os::Handle();
	}


	/**
	 * OStream.
	 */

	SocketOStream::SocketOStream(Socket *owner, os::Thread t) : HandleOStream(owner->handle, t) {}

	SocketOStream::~SocketOStream() {
		// The socket will close the handle.
		handle = os::Handle();
	}

	void SocketOStream::close() {
		owner->closeEnd(Socket::closeWrite);
		handle = os::Handle();
	}


}
