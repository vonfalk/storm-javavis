#include "stdafx.h"
#include "Test/Test.h"
#include "Core/Io/Url.h"
#include "Core/Str.h"

BEGIN_TEST(UrlTest, Core) {
	Engine &e = *gEngine;

	Url *root = parsePath(e, L"/home/dev/other/");
	Url *other = parsePath(e, L"/home/dev/foo/bar.txt");

	Url *relative = other->relative(root);
	CHECK_OBJ_EQ(relative, parsePath(e, L"../foo/bar.txt"));
	CHECK_OBJ_EQ(root->push(relative), other);
	CHECK_OBJ_EQ(other->title(), new (e) Str(L"bar"));
	CHECK_OBJ_EQ(other->ext(), new (e) Str(L"txt"));
	CHECK_OBJ_EQ(other->name(), new (e) Str(L"bar.txt"));

	Url *hidden = parsePath(e, L"/home/.hidden");
	CHECK_OBJ_EQ(hidden->name(), new (e) Str(L".hidden"));

	CHECK_OBJ_EQ(parsePath(e, L"/home/a/.././dev/other/"), root);

	// Windows network shares.
	CHECK_OBJ_EQ(parsePath(e, L"//host/share/foo"), parsePath(e, L"/host/share/foo"));

	// Windows paths.
	CHECK_OBJ_EQ(parsePath(e, L"C:/host/share/foo"), parsePath(e, L"/C:/host/share/foo"));

	// Check the 'executableFileUrl'...
	CHECK(executableFileUrl(e)->exists());

	// TODO: Test file IO.
} END_TEST
