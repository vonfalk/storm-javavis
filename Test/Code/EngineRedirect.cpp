#include "stdafx.h"
#include "Code/Binary.h"
#include "Code/Listing.h"

using namespace code;

Str *toStr(EnginePtr e, Int a, Int b) {
	StrBuf *to = new (e) StrBuf();
	*to << a << S(" + ") << b;
	return to->toS();
}

BEGIN_TEST(EngineRedirectTest, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Ref toCall = arena->external(S("toStr"), address(&toStr));

	Array<TypeDesc *> *params = new (e) Array<TypeDesc *>();
	params->push(intDesc(e));
	params->push(intDesc(e));
	Listing *l = arena->engineRedirect(ptrDesc(e), params, toCall, e.ref(Engine::rEngine));

	Binary *b = new (e) Binary(arena, l);
	typedef Str *(*Fn)(Int, Int);
	Fn fn = (Fn)b->address();

	CHECK_EQ(toS((*fn)(10, 20)), L"10 + 20");

} END_TEST
