#include "stdafx.h"
#include "Test/Test.h"
#include "Storm/Function.h"

int64 findResult(const String &name) {
	int64 r = 0;
	int64 pos = 1;
	for (nat i = name.size(); i > 0; i--) {
		wchar ch = name[i-1];
		if (ch >= '0' && ch <= '9') {
			r += (ch - '0') * pos;
			pos *= 10;
		} else if (ch == 'N') {
			r = -r;
		} else {
			break;
		}
	}
	return r;
}

BEGIN_TEST(EvalTest) {
	Engine &e = *gEngine;
	Package *pkg = e.package(L"test.eval");
	CHECK(pkg);
	if (!pkg)
		break;

	pkg->forceLoad();

	for (NameSet::iterator i = pkg->begin(); i != pkg->end(); i++) {
		Auto<Function> fn = i->as<Function>();
		if (!fn)
			continue;

		int64 nr = findResult(fn->name);
		if (fn->result == value<Byte>(e)) {
			CHECK_EQ_TITLE(fn->call<Byte>(), (Byte)nr, fn->identifier());
		} else if (fn->result == value<Int>(e)) {
			CHECK_EQ_TITLE(fn->call<Int>(), (Int)nr, fn->identifier());
		} else if (fn->result == value<Nat>(e)) {
			CHECK_EQ_TITLE(fn->call<Nat>(), (Nat)nr, fn->identifier());
		} else if (fn->result == value<Long>(e)) {
			CHECK_EQ_TITLE(fn->call<Long>(), (Long)nr, fn->identifier());
		} else if (fn->result == value<Word>(e)) {
			CHECK_EQ_TITLE(fn->call<Word>(), (Word)nr, fn->identifier());
		} else if (fn->result == value<Float>(e)) {
			CHECK_EQ_TITLE(fn->call<Float>(), (Float)nr, fn->identifier());
		}
	}

} END_TEST
