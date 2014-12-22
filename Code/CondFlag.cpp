#include "stdafx.h"
#include "CondFlag.h"

namespace code {

	const wchar_t *name(CondFlag c) {
		switch (c) {
		case ifAlways:
			return L"always";
		case ifNever:
			return L"never";
		case ifOverflow:
			return L"overflow";
		case ifNoOverflow:
			return L"no overflow";
		case ifEqual:
			return L"equal";
		case ifNotEqual:
			return L"not equal";
		case ifBelow:
			return L"below";
		case ifBelowEqual:
			return L"below/equal";
		case ifAboveEqual:
			return L"above/equal";
		case ifAbove:
			return L"above";
		case ifLess:
			return L"less";
		case ifLessEqual:
			return L"less/equal";
		case ifGreaterEqual:
			return L"greater/equal";
		case ifGreater:
			return L"greater";
		}

		TODO(L"Implement!");
		assert(false);
		return L"Unknown CondFlag";
	}

	CondFlag inverse(CondFlag c) {
		switch (c) {
		case ifAlways:
			return ifNever;
		case ifNever:
			return ifAlways;
		case ifOverflow:
			return ifNoOverflow;
		case ifNoOverflow:
			return ifOverflow;
		case ifEqual:
			return ifNotEqual;
		case ifNotEqual:
			return ifEqual;
		case ifBelow:
			return ifAboveEqual;
		case ifBelowEqual:
			return ifAbove;
		case ifAboveEqual:
			return ifBelow;
		case ifAbove:
			return ifBelowEqual;
		case ifLess:
			return ifGreaterEqual;
		case ifLessEqual:
			return ifGreater;
		case ifGreaterEqual:
			return ifLess;
		case ifGreater:
			return ifLessEqual;
		}

		TODO(L"Implement!");
		assert(false);
		return ifNever;
	}


}
