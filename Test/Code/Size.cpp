#include "stdafx.h"
#include "Code/Size.h"
#include "Utils/Memory.h"

BEGIN_TEST(SizeTest, GcScan) {
	Size s = Size::sInt;
	CHECK_EQ(s.current(), 4);
	s += Size::sByte;
	CHECK_EQ(s.current(), 8);
	s += Size::sByte;
	CHECK_EQ(s.current(), 8);
	s += Size::sInt;
	CHECK_EQ(s.current(), 12);
} END_TEST

struct STA {
	Long v;
};

struct STB {
	Bool a;
	STA b;
};

BEGIN_TEST(SizeTest2, GcScan) {
	Size s = Size::sByte;
	CHECK_EQ(s.current(), 1);
	// Note: as a long needs to be aligned to 8 bytes, 7 padding bytes will be inserted here.
	s += Size::sLong;
	CHECK_EQ(s.current(), 16);
	CHECK_EQ(s.current(), sizeof(STB));

	s = Size();
	s += Size::sByte;
	s += Size::sLong.alignment();
	CHECK_EQ(s.current(), OFFSET_OF(STB, b));
} END_TEST
