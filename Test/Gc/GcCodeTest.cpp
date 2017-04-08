#include "stdafx.h"
#include "Compiler/Debug.h"
#include "Utils/Bitwise.h"

using namespace storm::debug;

BEGIN_TEST(CodeAllocTest, GcObjects) {
	Engine &e = gEngine();

	nat count = 10;
	nat allocSize = sizeof(void *) * count + 3;

	void *code = runtime::allocCode(e, allocSize, count);

	GcCode *c = runtime::codeRefs(code);

	Array<PtrKey *> *o = new (e) Array<PtrKey *>();
	PtrKey **objs = (PtrKey **)code;

	for (nat i = 0; i < count; i++) {
		c->refs[i].offset = sizeof(void *)*i;
		c->refs[i].kind = GcCodeRef::rawPtr;

		PtrKey *k = new (e) PtrKey();
		c->refs[i].pointer = k;
		o->push(k);
	}
	runtime::codeUpdatePtrs(code);

	bool moved = false;
	for (nat i = 0; i < 20 && !moved; i++) {
		for (nat i = 0; i < o->count(); i++) {
			moved |= o->at(i)->moved();
		}

		createList(e, 100);
		e.gc.collect();
	}

	CHECK_EQ(runtime::codeSize(code), roundUp(size_t(allocSize), sizeof(size_t)));
	CHECK_EQ(runtime::codeRefs(code)->refCount, 10);

	for (nat i = 0; i < count; i++) {
		CHECK_EQ(o->at(i), objs[i]);
	}

} END_TEST

BEGIN_TEST(CodeRelPtr, GcObjects) {
	Engine &e = gEngine();

	static GcType type = {
		GcType::tArray,
		null,
		null,
		sizeof(void *),
		1, { 0 },
	};

	nat count = 20;
	GcArray<void **> *codeArray = runtime::allocArray<void **>(e, &type, count);

	for (nat i = 0; i < count; i++) {
		void **code = (void **)runtime::allocCode(e, sizeof(void *) * 2, 2);
		GcCode *meta = runtime::codeRefs(code);
		meta->refs[0].offset = 0;
		meta->refs[0].kind = GcCodeRef::inside;
		meta->refs[0].pointer = (void *)sizeof(void *); // point to code[1]
		// *code = code + 1;
		runtime::codeUpdatePtrs(code);

		codeArray->v[i] = code;
	}

	// Make stuff move!
	createList(e, 1000);
	e.gc.collect();

	// Verify stuff!
	for (nat i = 0; i < count; i++) {
		void **code = codeArray->v[i];

		// We shall still point into the object.
		CHECK_EQ(*code, code + 1);
	}

} END_TEST;
