#include "stdafx.h"
#include "Compiler/Named.h"

BEGIN_TEST(DocTest, Storm) {
	Engine &e = gEngine();

	Value intVal(StormInfo<Int>::type(e));

	SimpleName *name = parseSimpleName(e, S("core.debug.docFunction"));
	name->last()->params->push(intVal);
	Named *fn = e.scope().find(name);
	VERIFY(fn);
	VERIFY(fn->documentation);

	Doc *doc = fn->documentation->get();
	CHECK_EQ(doc->params->count(), 1);
	CHECK_EQ(doc->params->at(0).type, intVal);
	CHECK_EQ(*doc->params->at(0).name, S("param"));
	CHECK_EQ(*doc->name, S("docFunction"));
	CHECK_EQ(*doc->body, S("Check documentation"));

} END_TEST


BEGIN_TEST(Doc2Test, Storm) {
	Engine &e = gEngine();

	Value intVal(StormInfo<Int>::type(e));

	SimpleName *name = parseSimpleName(e, S("tests.bs.doc1Fn"));
	name->last()->params->push(intVal);
	Named *fn = e.scope().find(name);
	VERIFY(fn);
	VERIFY(fn->documentation);

	Doc *doc = fn->documentation->get();
	CHECK_EQ(doc->params->count(), 1);
	CHECK_EQ(doc->params->at(0).type, intVal);
	CHECK_EQ(*doc->params->at(0).name, S("a"));
	CHECK_EQ(*doc->name, S("doc1Fn"));
	CHECK_EQ(*doc->body, S("Check documentation"));

} END_TEST


BEGIN_TEST(Doc3Test, Storm) {
	Engine &e = gEngine();

	Value intVal(StormInfo<Int>::type(e));

	SimpleName *name = parseSimpleName(e, S("tests.bs.doc2Fn"));
	name->last()->params->push(intVal);
	Named *fn = e.scope().find(name);
	VERIFY(fn);
	VERIFY(fn->documentation);

	Doc *doc = fn->documentation->get();
	CHECK_EQ(doc->params->count(), 1);
	CHECK_EQ(doc->params->at(0).type, intVal);
	CHECK_EQ(*doc->params->at(0).name, S("b"));
	CHECK_EQ(*doc->name, S("doc2Fn"));
	CHECK_EQ(*doc->body, S("Check documentation"));

} END_TEST
