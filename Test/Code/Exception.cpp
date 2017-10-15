#include "stdafx.h"
#include "Code/Binary.h"
#include "Code/Listing.h"

using namespace code;

static Int destroyed = 0;

static void intCleanup(Int v) {
	destroyed += v;
}

static void intPtrCleanup(Int *v) {
	destroyed += *v;
}

static Int throwAt = 0;

static Int throwError(Int point) {
	if (point == throwAt) {
		throw Error();
	}
	return point;
}

BEGIN_TEST(CodeExceptionTest, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Ref intCleanup = arena->external(S("intCleanup"), address(&::intCleanup));
	Ref errorFn = arena->external(S("errorFn"), address(&::throwError));

	Listing *l = new (e) Listing();
	Block block = l->createBlock(l->root());
	Var v = l->createVar(l->root(), Size::sInt, intCleanup, freeOnException);
	Var w = l->createVar(block, Size::sInt, intCleanup, freeOnException);

	*l << prolog();

	*l << mov(v, intConst(10));
	*l << fnParam(intDesc(e), intConst(1));
	*l << fnCall(errorFn);

	*l << begin(block);
	*l << mov(w, intConst(20));
	*l << fnParam(intDesc(e), intConst(2));
	*l << fnCall(errorFn);
	*l << end(block);

	*l << fnParam(intDesc(e), intConst(3));
	*l << fnCall(errorFn);

	l->result = intDesc(e);
	*l << fnRet(eax);

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)();
	Fn fn = (Fn)b->address();

	throwAt = 1;
	destroyed = 0;
	CHECK_ERROR((*fn)(), Error);
	CHECK_EQ(destroyed, 10);

	throwAt = 2;
	destroyed = 0;
	CHECK_ERROR((*fn)(), Error);
	CHECK_EQ(destroyed, 30);

	throwAt = 3;
	destroyed = 0;
	CHECK_ERROR((*fn)(), Error);
	CHECK_EQ(destroyed, 10);

	throwAt = 4;
	destroyed = 0;
	CHECK_RUNS((*fn)());
	CHECK_EQ(destroyed, 0);

} END_TEST


BEGIN_TEST(CodeCleanupTest, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Ref intCleanup = arena->external(S("intCleanup"), address(&::intCleanup));
	Ref errorFn = arena->external(S("errorFn"), address(&::throwError));

	Listing *l = new (e) Listing();
	Block block = l->createBlock(l->root());
	Var v = l->createVar(l->root(), Size::sInt, intCleanup);
	Var w = l->createVar(block, Size::sInt, intCleanup);

	*l << prolog();

	*l << mov(v, intConst(10));
	*l << fnParam(intDesc(e), intConst(1));
	*l << fnCall(errorFn);

	*l << begin(block);
	*l << mov(w, intConst(20));
	*l << fnParam(intDesc(e), intConst(2));
	*l << fnCall(errorFn);
	*l << end(block);

	*l << fnParam(intDesc(e), intConst(3));
	*l << fnCall(errorFn);

	l->result = intDesc(e);
	*l << fnRet(eax);

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)();
	Fn fn = (Fn)b->address();

	throwAt = 1;
	destroyed = 0;
	CHECK_ERROR((*fn)(), Error);
	CHECK_EQ(destroyed, 10);

	throwAt = 2;
	destroyed = 0;
	CHECK_ERROR((*fn)(), Error);
	CHECK_EQ(destroyed, 30);

	throwAt = 3;
	destroyed = 0;
	CHECK_ERROR((*fn)(), Error);
	CHECK_EQ(destroyed, 30);

	throwAt = 4;
	destroyed = 0;
	CHECK_RUNS((*fn)());
	CHECK_EQ(destroyed, 30);

} END_TEST


BEGIN_TEST(ExceptionRefTest, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Ref intCleanup = arena->external(S("intCleanup"), address(&::intPtrCleanup));
	Ref errorFn = arena->external(S("errorFn"), address(&::throwError));

	Listing *l = new (e) Listing();
	Var v = l->createVar(l->root(), Size::sInt, intCleanup, freeOnBoth | freePtr);

	*l << prolog();

	*l << mov(v, intConst(103));
	*l << fnParam(intDesc(e), intConst(1));
	*l << fnCall(errorFn);

	l->result = intDesc(e);
	*l << fnRet(eax);

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)();
	Fn fn = (Fn)b->address();

	throwAt = 1;
	destroyed = 0;
	CHECK_ERROR((*fn)(), Error);
	CHECK_EQ(destroyed, 103);

	throwAt = 2;
	destroyed = 0;
	CHECK_RUNS((*fn)());
	CHECK_EQ(destroyed, 103);

} END_TEST


/**
 * See that we properly restore seh frames and similar things.
 */
#if defined(X86) && defined(WINDOWS)

BEGIN_TEST(ExceptionSehTest, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Ref intCleanup = arena->external(S("intCleanup"), &::intCleanup);
	Ref errorFn = arena->external(S("errorFn"), &::throwError);

	Listing *l = new (e) Listing();
	Block block = l->createBlock(l->root());
	Var v = l->createVar(l->root(), Size::sInt, intCleanup);

	*l << prolog();

	*l << mov(v, intConst(103));
	*l << fnParam(intConst(1));
	*l << fnCall(errorFn, valInt());

	l->result = intDesc(e);
	*l << fnRet(eax);

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)();
	Fn fn = (Fn)b->address();

	void *sehTop, *after;
	__asm {
		mov eax, fs:[0];
		mov sehTop, eax;
	};

	throwAt = 0;
	destroyed = 0;
	CHECK_RUNS((*fn)());

	__asm {
		mov eax, fs:[0];
		mov after, eax;
	};

	CHECK_EQ(after, sehTop);

	// Crashes if the above check fails...
	throwAt = 1;
	destroyed = 0;
	CHECK_ERROR((*fn)(), Error);
} END_TEST

#endif


struct Large {
	int a;
	int b;
	int c;

	Large() : a(0), b(10), c(0) {}
	~Large();
};

static bool correct = false;
static int times = 0;

static void destroyLarge(Large *large) {
	correct &= large->a == 0;
	correct &= large->b == 10;
	correct &= large->c == 0;
	times++;
}

Large::~Large() {
	destroyLarge(this);
}

static void copyLarge(Large *dest, Large *src) {
	*dest = *src;
}

BEGIN_TEST(ExceptionLargeTest, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Ref largeCopy = arena->external(S("largeCopy"), address(&::copyLarge));
	Ref largeCleanup = arena->external(S("largeCleanup"), address(&::destroyLarge));
	Ref errorFn = arena->external(S("errorFn"), address(&::throwError));

	Listing *l = new (e) Listing();
	ComplexDesc *desc = new (e) ComplexDesc(Size::sInt * 3, largeCopy, largeCleanup);
	Var p = l->createParam(desc);
	Var v = l->createVar(l->root(), Size::sInt * 3, largeCleanup, freeOnBoth | freePtr);

	*l << prolog();

	*l << mov(intRel(v, Offset()), intRel(p, Offset()));
	*l << mov(intRel(v, Offset::sInt), intRel(p, Offset::sInt));
	*l << mov(intRel(v, Offset::sInt * 2), intRel(p, Offset::sInt * 2));
	*l << fnParam(intDesc(e), intConst(1));
	*l << fnCall(errorFn);

	l->result = intDesc(e);
	*l << fnRet(eax);

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)(Large);
	Fn fn = (Fn)b->address();

	Large large;

	throwAt = 1;
	correct = true;
	times = 0;
	CHECK_ERROR((*fn)(large), Error);
	CHECK(correct);
	CHECK_EQ(times, 2);

	throwAt = 2;
	correct = true;
	times = 0;
	CHECK_RUNS((*fn)(large));
	CHECK(correct);
	CHECK_EQ(times, 2);

} END_TEST
