#include "stdafx.h"
#include "Core/Io/Net/Address.h"

BEGIN_TEST_(NetTest) {
	Engine &e = gEngine();

	Address *addr = new (e) Inet4Address(0, 0xC0A80101);
	CHECK_EQ(toS(addr), L"192.168.1.1");

	addr = new (e) Inet6Address(0, 0, 0, 0, 1);
	CHECK_EQ(toS(addr), L"::01");
	addr = new (e) Inet6Address(0, 0xFF000000, 0, 0, 0);
	CHECK_EQ(toS(addr), L"FF::");

} END_TEST
