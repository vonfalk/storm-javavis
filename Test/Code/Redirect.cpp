#include "stdafx.h"
#include "Code/Binary.h"
#include "Code/Listing.h"
#include "Code/Redirect.h"

using namespace code;

static Int destroyed = 0;

class BigType {
public:
	BigType(Int v) : v(v) {}
	~BigType() {
		destroyed += v;
	}

	Int v;
};

static void freeBig(BigType *me) {
	me->~BigType();
}

static void copyBig(BigType *to, BigType *from) {
	new (to) BigType(*from);
}

static BigType redirectTo(BigType v) {
	return BigType(v.v + 10);
}

static bool throwError = false;

static const void *redirectFn(Int v) {
	if (throwError)
		throw Error();
	return address(&redirectTo);
}


BEGIN_TEST(RedirectTest, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Ref copyBig = arena->external(S("copyBig"), address(&::copyBig));
	Ref freeBig = arena->external(S("freeBig"), address(&::freeBig));
	Ref redirectFn = arena->external(S("redirectFn"), address(&::redirectFn));

	TypeDesc *bigDesc = new (e) ComplexDesc(Size::sInt, copyBig, freeBig);

	Array<TypeDesc *> *params = new (e) Array<TypeDesc *>();
	params->push(bigDesc);

	Listing *l = arena->redirect(false, bigDesc, params, redirectFn, Operand());

	Binary *b = new (e) Binary(arena, l);
	typedef BigType (*Fn)(BigType);
	Fn fn = (Fn)b->address();

	throwError = true;
	destroyed = 0;
	CHECK_ERROR((*fn)(BigType(10)), Error);
	CHECK_EQ(destroyed, 10);

	throwError = false;
	CHECK_EQ((*fn)(BigType(10)).v, 20);

} END_TEST
