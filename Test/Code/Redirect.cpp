#include "stdafx.h"
#include "Code/Binary.h"
#include "Code/Listing.h"
#include "Code/Redirect.h"

using namespace code;

static Int redirectTo(Int v) {
	return v + 10;
}

static bool throwError = false;

static void *redirectFn(Int v) {
	if (throwError)
		throw Error();
	return &redirectTo;
}

static Int destroyed = 0;

static void destroyInt(Int v) {
	destroyed += v;
}

BEGIN_TEST(RedirectTest, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Ref freeInt = arena->external(L"freeInt", &destroyInt);
	Ref redirectFn = arena->external(L"redirectFn", &::redirectFn);

	Array<RedirectParam> *p = new (e) Array<RedirectParam>();
	p->push(RedirectParam(valInt(), freeInt, false));
	Listing *l = redirect(p, redirectFn, Operand());

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)(Int);
	Fn fn = (Fn)b->address();

	throwError = true;
	destroyed = 0;
	CHECK_ERROR((*fn)(10), Error);
	CHECK_EQ(destroyed, 10);

	throwError = false;
	CHECK_EQ((*fn)(10), 20);

} END_TEST
