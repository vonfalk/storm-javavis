#include "stdafx.h"
#include "Fn.h"
#include "Compiler/Package.h"
#include "Compiler/Function.h"

template <class T>
static T callFn(Function *f) {
	typedef T (*Ptr)();
	Ptr p = (Ptr)f->ref().address();
	return (*p)();
}

static int64 findResult(const Str *name) {
	const wchar *str = name->c_str();
	nat len = wcslen(str);
	int64 r = 0;
	int64 pos = 1;

	for (nat i = len; i > 0; i--) {
		wchar ch = str[i-1];
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

static bool findBoolResult(const Str *name) {
	const wchar *str = name->c_str();
	wchar last = str[wcslen(str) - 1];
	switch (last) {
	case 'T':
		return true;
	case 'F':
		return false;
	default:
		throw TestError(L"Tests returning booleans must end with either T or F!");
	}
}

BEGIN_TEST(EvalTest, BS) {
	Engine &e = gEngine();
	Package *pkg = e.package(L"test.eval");
	VERIFY(pkg);

	pkg->forceLoad();
	for (NameSet::Iter i = pkg->begin(); i != pkg->end(); i++) {
		Function *fn = as<Function>(i.v());
		if (!fn)
			continue;

		if (fn->result.type == StormInfo<Bool>::type(e)) {
			bool b = findBoolResult(fn->name);
			CHECK_EQ_TITLE(callFn<Bool>(fn), b, fn->identifier());
			continue;
		}

		int64 nr = findResult(fn->name);
		if (fn->result.type == StormInfo<Byte>::type(e)) {
			CHECK_EQ_TITLE(callFn<Byte>(fn), (Byte)nr, fn->identifier());
		} else if (fn->result.type == StormInfo<Int>::type(e)) {
			CHECK_EQ_TITLE(callFn<Int>(fn), (Int)nr, fn->identifier());
		} else if (fn->result.type == StormInfo<Nat>::type(e)) {
			CHECK_EQ_TITLE(callFn<Nat>(fn), (Nat)nr, fn->identifier());
		} else if (fn->result.type == StormInfo<Long>::type(e)) {
			CHECK_EQ_TITLE(callFn<Long>(fn), (Long)nr, fn->identifier());
		} else if (fn->result.type == StormInfo<Word>::type(e)) {
			CHECK_EQ_TITLE(callFn<Word>(fn), (Word)nr, fn->identifier());
		} else if (fn->result.type == StormInfo<Float>::type(e)) {
			CHECK_EQ_TITLE(callFn<Float>(fn), (Float)nr, fn->identifier());
		} else {
			throw TestError(L"Unknown return type for function " + ::toS(fn->identifier()));
		}
	}
} END_TEST
