#include "stdafx.h"
#include "NetStream.h"

namespace storm {

	NetStream::NetStream(os::Handle handle, os::Thread attachTo, Address *peer)
		: Socket(handle, attachedTo), closed(0), peer(peer) {

		i = new (this) NetIStream(this, attachTo);
		o = new (this) NetOStream(this, attachTo);
	}

	void NetStream::deepCopy(CloneEnv *env) {
		cloned(i, env);
		cloned(o, env);
	}

	NetIStream *NetStream::input() const {
		return i;
	}

	NetOStream *NetStream::output() const {
		return o;
	}

	Bool NetStream::nodelay() const {
		int v = 0;
		getSocketOpt(handle, IPPROTO_TCP, TCP_NODELAY, &v, sizeof(v));
		return v != 0;
	}

	void NetStream::nodelay(Bool v) {
		int val = v ? 1 : 0;
		setSocketOpt(handle, IPPROTO_TCP, TCP_NODELAY, &val, sizeof(val));
	}

	void NetStream::closeEnd(Nat which) {
		Nat old, w;
		do {
			old = atomicRead(closed);
			w = old | which;
		} while (atomicCAS(closed, old, w) != old);

		if (w == (closeRead | closeWrite) && handle) {
			closeSocket(handle, attachedTo);
			handle = os::Handle();
		}
	}

	void NetStream::toS(StrBuf *to) const {
		Socket::toS(to);
		if (handle)
			*to << S(" <-> ") << peer;
	}

	/**
	 * Connect.
	 */

	MAYBE(NetStream *) connect(Address *to) {
		initSockets();

		sockaddr_storage data;
		sockaddr *sa = (sockaddr *)&data;
		to->fill(sa);

		os::Handle h = createTcpSocket(sa->sa_family);
		os::Thread current = os::Thread::current();
		current.attach(h);

		try {
			// Connect!
			bool ok = connectSocket(h, current, sa, sizeof(data));
			if (!ok)
				return null;

			return new (to) NetStream(h, current, to);
		} catch (...) {
			closeSocket(h, current);
			throw;
		}
	}

	MAYBE(NetStream *) connect(Str *host, Nat port) {
		Array<Address *> *found = lookupAddress(host);

		for (Nat i = 0; i < found->count(); i++) {
			Address *addr = found->at(i);
			if (!addr->port())
				addr = addr->withPort(port);

			if (NetStream *s = connect(addr))
				return s;
		}

		return null;
	}


	/**
	 * IStream.
	 */

	NetIStream::NetIStream(NetStream *owner, os::Thread t) : HandleIStream(owner->handle, t) {}

	NetIStream::~NetIStream() {
		// The socket will close the handle.
		handle = os::Handle();
	}

	void NetIStream::close() {
		owner->closeEnd(NetStream::closeRead);
		handle = os::Handle();
	}


	/**
	 * OStream.
	 */

	NetOStream::NetOStream(NetStream *owner, os::Thread t) : HandleOStream(owner->handle, t) {}

	NetOStream::~NetOStream() {
		// The socket will close the handle.
		handle = os::Handle();
	}

	void NetOStream::close() {
		owner->closeEnd(NetStream::closeWrite);
		handle = os::Handle();
	}


}
