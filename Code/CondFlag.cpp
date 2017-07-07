#include "stdafx.h"
#include "CondFlag.h"

namespace code {

	const wchar *name(CondFlag c) {
		switch (c) {
		case ifAlways:
			return S("always");
		case ifNever:
			return S("never");
		case ifOverflow:
			return S("overflow");
		case ifNoOverflow:
			return S("no overflow");
		case ifEqual:
			return S("equal");
		case ifNotEqual:
			return S("not equal");
		case ifBelow:
			return S("below");
		case ifBelowEqual:
			return S("below/equal");
		case ifAboveEqual:
			return S("above/equal");
		case ifAbove:
			return S("above");
		case ifLess:
			return S("less");
		case ifLessEqual:
			return S("less/equal");
		case ifGreaterEqual:
			return S("greater/equal");
		case ifGreater:
			return S("greater");
		case ifFBelow:
			return S("ifFBelow");
		case ifFBelowEqual:
			return S("ifFBelowEqual");
		case ifFAboveEqual:
			return S("ifFAboveEqual");
		case ifFAbove:
			return S("ifFAbove");
		}

		TODO(L"Implement!");
		assert(false);
		return S("Unknown CondFlag");
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
		case ifFBelow:
			return ifFAboveEqual;
		case ifFBelowEqual:
			return ifFAbove;
		case ifFAboveEqual:
			return ifFBelow;
		case ifFAbove:
			return ifFBelowEqual;
		}

		TODO(L"Implement!");
		assert(false);
		return ifNever;
	}

}
