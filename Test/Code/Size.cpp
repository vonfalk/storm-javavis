#include "stdafx.h"
#include "Code/Size.h"

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
