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
		if (Address *r = toStormUnsafe(e, addr))
			return r;

		throw NetError(L"Unsupported address family!");
	}

	Address *toStormUnsafe(Engine &e, sockaddr *addr) {
		switch (addr->sa_family) {
		case AF_INET:
			return new (e) Inet4Address((sockaddr_in *)addr);
		case AF_INET6:
			return new (e) Inet6Address((sockaddr_in6 *)addr);
		default:
			return null;
		}
	}

	static void error(Str *src) {
		throw NetError(L"Invalid address: " + ::toS(src));
	}

	static Address *to4Address(Str *str) {
		const wchar *src = str->c_str();

		Nat number = 0;
		Nat digits = 0;
		Nat addr = 0;
		Nat parts = 0;

		Bool hasPort = false;
		Nat port = 0;

		for (const wchar *at = src; *at; at++) {
			if (*at >= '0' && *at <= '9') {
				number = number * 10 + (*at - '0');
				digits++;
			} else if (*at == '.') {
				if (digits == 0 || number > 0xFF || parts > 3 || hasPort)
					error(str);

				parts++;
				addr = (addr << 8) | number;
				digits = 0;
				number = 0;
			} else if (*at == ':') {
				if (digits == 0 || number > 0xFF || parts > 3 || hasPort)
					error(str);

				hasPort = true;
				addr = (addr << 8) | number;
				digits = 0;
				number = 0;
			} else {
				error(str);
			}
		}

		if (digits == 0)
			error(str);

		if (hasPort) {
			port = number;
		} else if (number > 0xFF || parts > 3) {
			error(str);
		} else {
			addr = (addr << 8) | number;
		}

		return new (str) Inet4Address(port, addr);
	}

	static Bool isHex(wchar c) {
		return (c >= '0' && c <= '9')
			|| (c >= 'A' && c <= 'F')
			|| (c >= 'a' && c <= 'f');
	}

	static Nat fromHex(wchar c) {
		if (c >= '0' && c <= '9')
			return c - '0';
		if (c >= 'A' && c <= 'F')
			return c - 'A' + 10;
		if (c >= 'a' && c <= 'f')
			return c - 'a' + 10;
		return 0;
	}

	static Address *to6Address(Str *str) {
		const wchar *src = str->c_str();

		Nat parts[8] = { 0 };
		Nat part = 0;
		Nat digits = 0;
		Nat expand = 8; // no expansion

		Nat port = 0;
		Bool hasPort = false;
		Bool inPort = false;

		for (const wchar *at = src; *at; at++) {
			if (!inPort && isHex(*at)) {
				if (part > 8)
					error(str);

				parts[part] = parts[part] * 0x10 + fromHex(*at);
				digits++;
			} else if (inPort && *at > '0' && *at < '9') {
				if (hasPort)
					// no ':' yet
					error(str);

				port = port * 10 + (*at - '0');
				digits++;
			} else if (*at == ':') {
				if (inPort) {
					// Indicate that we're all done!
					hasPort = false;
				} else if (digits == 0) {
					if (at != src) {
						// Double!
						if (expand < 8)
							error(str);
						expand = part;
					}
				} else {
					part++;
				}
				digits = 0;
			} else if (*at == '[') {
				if (at != src)
					error(str);
				hasPort = true;
			} else if (*at == ']') {
				if (digits == 0) {
					if (expand >= 8)
						error(str);
				} else {
					part++;
				}

				if (!hasPort)
					error(str);
				inPort = true;
			} else {
				error(str);
			}
		}

		if (digits == 0) {
			if (expand >= 8 || inPort)
				error(str);
		} else if (!inPort) {
			part++;
		}

		if (part != 8 && expand >= 8)
			error(str);

		if (expand < 8) {
			// 'i' is index from the end.
			for (Nat i = 0; i < 8 - expand; i++) {
				Nat to = 8 - i - 1;
				if (i < part - expand)
					parts[to] = parts[part - i - 1];
				else
					parts[to] = 0;
			}
		}

		return new (str) Inet6Address(port,
									(parts[0] << 16) | parts[1],
									(parts[2] << 16) | parts[3],
									(parts[4] << 16) | parts[5],
									(parts[6] << 16) | parts[7]);
	}


	Address *toAddress(Str *str) {
		const wchar *src = str->c_str();

		bool dot = false;
		for (const wchar *i = src; *i; i++)
			if (*i == '.')
				dot = true;

		if (dot)
			return to4Address(str);
		else
			return to6Address(str);
	}

#if defined(WINDOWS)

	static bool doLookup(Str *str, Array<Address *> *to) {
		initSockets();

		ADDRINFOW *results = NULL;
		if (GetAddrInfoW(str->c_str(), NULL, NULL, &results) != 0)
			return false;

		for (ADDRINFOW *at = results; at; at = at->ai_next) {
			if (at->ai_addr)
				if (Address *addr = toStormUnsafe(str->engine(), at->ai_addr))
					to->push(addr);
		}

		FreeAddrInfoW(results);
		return true;
	}

#elif defined(POSIX)

	static bool doLookup(Str *str, Array<Address *> *to) {
		addrinfo *results = NULL;
		if (getaddrinfo(str->utf8_str(), NULL, NULL, &results) != 0)
			return false;

		for (addrinfo *at = results; at; at = at->ai_next) {
			if (at->ai_addr)
				if (Address *addr = toStormUnsafe(str->engine(), at->ai_addr))
					to->push(addr);
		}

		freeaddrinfo(results);
		return true;
	}

#endif

	Array<Address *> *lookupAddress(Str *str) {
		Nat port = 0;

		Str::Iter colon = str->findLast(Char(':'));
		if (colon != str->end()) {
			Str::Iter portStart = colon;
			Str *portStr = str->substr(++portStart);
			if (portStr->isNat()) {
				port = portStr->toNat();
				str = str->substr(str->begin(), colon);
			}
		}

		Array<Address *> *result = new (str) Array<Address *>();
		if (!doLookup(str, result))
			return result;

		if (port) {
			for (Nat i = 0; i < result->count(); i++) {
				result->at(i) = result->at(i)->withPort(port);
			}
		}

		return result;
	}

	/**
	 * IPv4.
	 */

	Inet4Address::Inet4Address(sockaddr_in *addr) : Address(ntohs(addr->sin_port)) {
		memcpy(&myAddr, &addr->sin_addr, sizeof(myAddr));
		myAddr = ntohl(myAddr);
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

		Nat tmp = htonl(myAddr);
		memcpy(&i->sin_addr, &tmp, sizeof(myAddr));
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

		Nat data[4];
		memcpy(data, &addr->sin6_addr, 4*sizeof(myAddr0));
		myAddr0 = ntohl(data[0]);
		myAddr1 = ntohl(data[1]);
		myAddr2 = ntohl(data[2]);
		myAddr3 = ntohl(data[3]);
	}

	Inet6Address::Inet6Address(Nat port, Nat addr0, Nat addr1, Nat addr2, Nat addr3)
		: Address(port), myAddr0(addr0), myAddr1(addr1), myAddr2(addr2), myAddr3(addr3), myFlow(0), myScope(0) {}

	Inet6Address::Inet6Address(Nat port, Nat addr0, Nat addr1, Nat addr2, Nat addr3, Nat flow, Nat scope)
		: Address(port), myAddr0(addr0), myAddr1(addr1), myAddr2(addr2), myAddr3(addr3), myFlow(flow), myScope(scope) {}

	Nat Inet6Address::operator [](Nat id) const {
		if (id >= 16)
			return 0;

		const Nat *src[] = { &myAddr0, &myAddr1, &myAddr2, &myAddr3 };
		const Nat part = *src[id / 2];
		return (part >> (16 - (id % 2)*16)) & 0xFFFF;
	}

	void Inet6Address::fill(sockaddr *fill) const {
		sockaddr_in6 *i = (sockaddr_in6 *)fill;
		i->sin6_family = AF_INET6;
		i->sin6_port = htons((short)port());
		i->sin6_flowinfo = htonl(myFlow);
		i->sin6_scope_id = htonl(myScope);

		Nat data[4] = { htonl(myAddr0), htonl(myAddr1), htonl(myAddr2), htonl(myAddr3) };
		memcpy(&i->sin6_addr, data, 4*sizeof(myAddr0));
	}

	static void putHex(StrBuf *to, Nat part) {
		const wchar digits[] = S("0123456789abcdef");
		wchar out[5];
		Nat at = 5;
		out[--at] = 0;

		while (part) {
			out[--at] = digits[part & 0x0F];
			part >>= 4;
		}

		while (out[at] == '0')
			at++;

		if (at == 4)
			at = 3;

		*to << &out[at];
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
			putHex(to, (*this)[i]);
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
