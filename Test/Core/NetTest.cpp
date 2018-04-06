#include "stdafx.h"
#include "Core/Net/Address.h"
#include "Core/Net/Socket.h"

BEGIN_TEST(NetAddrTest, Core) {
	Engine &e = gEngine();

	Address *addr = new (e) Inet4Address(0, 0xC0A80101);
	CHECK_EQ(toS(addr), L"192.168.1.1");

	{
		sockaddr_in in;
		in.sin_family = AF_INET;
		in.sin_port = 0;
		inet_pton(AF_INET, "192.168.1.1", &in.sin_addr);
		Address *o = toStorm(e, (sockaddr *)&in);
		CHECK_EQ(toS(o), L"192.168.1.1");
		CHECK_EQ(*o, *addr);
	}

	addr = new (e) Inet6Address(0, 0, 0, 0, 0);
	CHECK_EQ(toS(addr), L"::");
	addr = new (e) Inet6Address(0, 0, 0, 0, 1);
	CHECK_EQ(toS(addr), L"::1");
	addr = new (e) Inet6Address(0, 0xEFEF000, 0, 0, 0);
	CHECK_EQ(toS(addr), L"efe:f000::");
	addr = new (e) Inet6Address(0, 0xFE0000, 0, 0, 1);
	CHECK_EQ(toS(addr), L"fe::1");

	{
		sockaddr_in6 in;
		memset(&in, 0, sizeof(in));
		in.sin6_family = AF_INET6;
		inet_pton(AF_INET6, "fe::1", &in.sin6_addr);
		Address *o = toStorm(e, (sockaddr *)&in);
		CHECK_EQ(toS(o), L"fe::1");
		CHECK_EQ(*o, *addr);
	}


	CHECK_EQ(toS(toAddress(new (e) Str(S("192.168.0.1")))), L"192.168.0.1");
	CHECK_EQ(toS(toAddress(new (e) Str(S("192.168.0.1:31337")))), L"192.168.0.1:31337");

	CHECK_EQ(toS(toAddress(new (e) Str(S("abcd:ef01:2345:6789:1234:5678:9ABC:1230")))), L"abcd:ef01:2345:6789:1234:5678:9abc:1230");
	CHECK_EQ(toS(toAddress(new (e) Str(S("::")))), L"::");
	CHECK_EQ(toS(toAddress(new (e) Str(S("::1")))), L"::1");
	CHECK_EQ(toS(toAddress(new (e) Str(S("efe:f::1")))), L"efe:f::1");
	CHECK_EQ(toS(toAddress(new (e) Str(S("1::fed:1")))), L"1::fed:1");

	CHECK_EQ(toS(toAddress(new (e) Str(S("[1::fed:1]:31337")))), L"[1::fed:1]:31337");

} END_TEST

BEGIN_TEST_(NetConnectTest, Core) {
	Engine &e = gEngine();

	Socket *s = connect(toAddress(new (e) Str(S("51.15.180.4:80"))));
	PVAR(s);

} END_TEST
