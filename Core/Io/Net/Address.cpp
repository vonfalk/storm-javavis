#include "stdafx.h"
#include "Address.h"
#include "Core/Hash.h"
#include "Core/CloneEnv.h"

namespace storm {

	Address::Address(Nat port) : myPort(port & 0xFFFF) {}

	Address *Address::withPort(Nat port) const {
		Address *c = clone(this);
		c->myPort = port & 0xFFFF;
		return c;
	}

	void Address::fill(sockaddr *fill) const {
		assert(false, L"Not implemented!");
	}

	void Address::toS(StrBuf *to) const {
		*to << S("?:") << port();
	}

	Nat Address::hash() const {
		return natHash(myPort);
	}

	Bool Address::operator ==(const Address &other) const {
		if (runtime::typeOf(this) != runtime::typeOf(&other))
			return false;

		return myPort == other.myPort;
	}

	Address *toStorm(Engine &e, sockaddr *addr) {
		switch (addr->sa_family) {
		case AF_INET:
			return new (e) Inet4Address((sockaddr_in *)addr);
		case AF_INET6:
			return new (e) Inet6Address((sockaddr_in6 *)addr);
		default:
			throw NetError(L"Unsupported address family!");
		}
	}

	/**
	 * IPv4.
	 */

	Inet4Address::Inet4Address(sockaddr_in *addr) : Address(ntohs(addr->sin_port)) {
		memcpy(&myAddr, &addr->sin_addr, sizeof(myAddr));
	}

	Inet4Address::Inet4Address(Nat port, Nat addr) : Address(port), myAddr(addr) {}

	Byte Inet4Address::operator [](Nat id) const {
		if (id >= 4)
			return 0;

		return Byte((myAddr >> (24 - id*8)) & 0xFF);
	}

	void Inet4Address::fill(sockaddr *fill) const {
		sockaddr_in *i = (sockaddr_in *)fill;
		i->sin_family = AF_INET;
		i->sin_port = htons((short)port());

		memcpy(&i->sin_addr, &myAddr, sizeof(myAddr));
	}

	void Inet4Address::toS(StrBuf *to) const {
		*to << (*this)[0] << S(".") << (*this)[1] << S(".") << (*this)[2] << S(".") << (*this)[3];
		if (port())
			*to << S(":") << port();
	}

	Nat Inet4Address::hash() const {
		return Address::hash() ^ natHash(myAddr);
	}

	Bool Inet4Address::operator ==(const Address &other) const {
		if (!Address::operator ==(other))
			return false;

		const Inet4Address &o = (const Inet4Address &)other;
		return myAddr == o.myAddr;
	}

	/**
	 * IPv6
	 */

	Inet6Address::Inet6Address(sockaddr_in6 *addr) : Address(addr->sin6_port) {
		myFlow = ntohl(addr->sin6_flowinfo);
		myScope = ntohl(addr->sin6_scope_id);
		memcpy(&myAddr0, &addr->sin6_addr, 4*sizeof(myAddr0));
	}

	Inet6Address::Inet6Address(Nat port, Nat addr0, Nat addr1, Nat addr2, Nat addr3)
		: Address(port), myAddr0(addr0), myAddr1(addr1), myAddr2(addr2), myAddr3(addr3), myFlow(0), myScope(0) {}

	Inet6Address::Inet6Address(Nat port, Nat addr0, Nat addr1, Nat addr2, Nat addr3, Nat flow, Nat scope)
		: Address(port), myAddr0(addr0), myAddr1(addr1), myAddr2(addr2), myAddr3(addr3), myFlow(flow), myScope(scope) {}

	Byte Inet6Address::operator [](Nat id) const {
		if (id >= 16)
			return 0;

		const Nat *src[] = { &myAddr0, &myAddr1, &myAddr2, &myAddr3 };
		const Nat part = *src[id / 4];
		return Byte((part >> (24 - (id%4)*8)) & 0xFF);
	}

	void Inet6Address::fill(sockaddr *fill) const {
		sockaddr_in6 *i = (sockaddr_in6 *)fill;
		i->sin6_family = AF_INET6;
		i->sin6_port = htons((short)port());
		i->sin6_flowinfo = htonl(myFlow);
		i->sin6_scope_id = htonl(myScope);

		memcpy(&i->sin6_addr, &myAddr0, 4*sizeof(myAddr0));
	}

	void Inet6Address::toS(StrBuf *to) const {
		if (port())
			*to << S("[");

		Nat largestStart = 0;
		Nat largestEnd = 0;

		{
			Nat currStart = 0;
			for (Nat i = 0; i < count(); i++) {
				if ((*this)[i]) {
					if (i - currStart > largestEnd) {
						largestStart = currStart;
						largestEnd = i;
					}

					currStart = i + 1;
				}
			}

			if (count() - currStart > largestEnd) {
				largestStart = currStart;
				largestEnd = count();
			}
		}

		for (Nat i = 0; i < count(); i++) {
			if (i >= largestStart && i < largestEnd && largestStart != largestEnd) {
				if (i == largestStart)
					*to << S(":");
				continue;
			}

			if (i > 0)
				*to << S(":");
			Byte d = (*this)[i];
			*to << hex(d);
		}

		if (largestEnd == count())
			*to << S(":");

		if (port())
			*to << S("]:") << port();

		if (myFlow)
			*to << S(",") << myFlow;
		if (myScope)
			*to << S(",") << myScope;
	}

	Nat Inet6Address::hash() const {
		Nat r = Address::hash();
		r ^= natHash(myAddr0);
		r ^= natHash(myAddr1);
		r ^= natHash(myAddr2);
		r ^= natHash(myAddr3);
		r ^= natHash(myScope);
		r ^= natHash(myFlow);
		return r;
	}

	Bool Inet6Address::operator ==(const Address &other) const {
		if (!Address::operator ==(other))
			return false;

		const Inet6Address &o = (const Inet6Address &)other;
		return myAddr0 == o.myAddr0
			&& myAddr1 == o.myAddr1
			&& myAddr2 == o.myAddr2
			&& myAddr3 == o.myAddr3
			&& myScope == o.myScope
			&& myFlow == o.myFlow;
	}

}
