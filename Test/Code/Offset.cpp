#include "stdafx.h"
#include "Code/Binary.h"
#include "Code/Listing.h"
#include "Code/OffsetSource.h"
#include "Code/OffsetReference.h"

using namespace code;

BEGIN_TEST(OffsetRefTest, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Int data[10] = { 0 };

	StrOffsetSource *ref0 = new (e) StrOffsetSource(S("ref0"));
	StrOffsetSource *ref1 = new (e) StrOffsetSource(S("ref1"));
	StrOffsetSource *ref2 = new (e) StrOffsetSource(S("ref2"));
	ref0->set(Offset::sInt);
	ref1->set(Offset::sByte);
	ref2->set(Offset::sInt * 5);

	Listing *l = new (e) Listing();
	Var param = l->createParam(ptrDesc(e));

	*l << prolog();

	*l << mov(ptrA, param);
	*l << mov(intRel(ptrA, ref2), intConst(10));

	*l << mov(eax, intConst(ref0));
	*l << add(eax, intConst(ref1));

	l->result = intDesc(e);
	*l << fnRet(eax);

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)(Int *);
	Fn fn = (Fn)b->address();

	Int r = (*fn)(data);
	CHECK_EQ(r, 5);
	CHECK_EQ(data[5], 10);

} END_TEST
