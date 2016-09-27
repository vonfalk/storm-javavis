#include "stdafx.h"
#include "Test/Lib/Test.h"
#include "Shared/Io/Url.h"
#include "Shared/Io/Text.h"

BEGIN_TEST(UrlTest) {

	Engine &e = *gEngine;

	Auto<Url> root = parsePath(e, L"/home/dev/other/");
	Auto<Url> other = parsePath(e, L"/home/dev/foo/bar.txt");

	Auto<Url> relative = other->relative(root);
	CHECK_OBJ_EQ(other->relative(root), parsePath(e, L"../foo/bar.txt"));
	CHECK_OBJ_EQ(root->push(relative), other);
	CHECK_OBJ_EQ(other->title(), CREATE(Str, e, L"bar"));
	CHECK_OBJ_EQ(other->ext(), CREATE(Str, e, L"txt"));
	CHECK_OBJ_EQ(other->name(), CREATE(Str, e, L"bar.txt"));

	Auto<Url> hidden = parsePath(e, L"/home/.hidden");
	CHECK_OBJ_EQ(hidden->name(), CREATE(Str, e, L".hidden"));

	CHECK_OBJ_EQ(parsePath(e, L"/home/a/.././dev/other/"), root);

	// Windows network shares.
	CHECK_OBJ_EQ(parsePath(e, L"//host/share/foo"), parsePath(e, L"/host/share/foo"));

	// Windows paths.
	CHECK_OBJ_EQ(parsePath(e, L"C:/host/share/foo"), parsePath(e, L"/C:/host/share/foo"));

	// Check the 'executableFileUrl'...
	CHECK(steal(executableFileUrl(e))->exists());

	Auto<Url> testFile = dbgRootUrl(e);
	testFile = testFile->push(L"TestData");
	testFile = testFile->push(L"SrcPos.txt");
	CHECK(testFile->exists());

	Auto<storm::IStream> stream = testFile->read();
	Auto<TextReader> text = readText(stream);
	CHECK_OBJ_EQ(text->readLine(), CREATE(Str, e, L"Sample text file"));

} END_TEST
