#include "stdafx.h"
#include "Core/Io/Url.h"
#include "Core/Str.h"

BEGIN_TEST(UrlTest, Core) {
	Engine &e = gEngine();

	Url *root = parsePath(e, S("/home/dev/other/"));
	Url *other = parsePath(e, S("/home/dev/foo/bar.txt"));

	Url *relative = other->relative(root);
	CHECK_OBJ_EQ(relative, parsePath(e, S("../foo/bar.txt")));
	CHECK_OBJ_EQ(root->push(relative), other);
	CHECK_OBJ_EQ(other->title(), new (e) Str(S("bar")));
	CHECK_OBJ_EQ(other->ext(), new (e) Str(S("txt")));
	CHECK_OBJ_EQ(other->name(), new (e) Str(S("bar.txt")));

	Url *hidden = parsePath(e, S("/home/.hidden"));
	CHECK_OBJ_EQ(hidden->name(), new (e) Str(S(".hidden")));

	CHECK_OBJ_EQ(parsePath(e, S("/home/a/.././dev/other/")), root);

	// Windows network shares.
	CHECK_OBJ_EQ(parsePath(e, S("//host/share/foo")), parsePath(e, S("/host/share/foo")));

	// Windows paths.
	CHECK_OBJ_EQ(parsePath(e, S("C:/host/share/foo")), parsePath(e, S("/C:/host/share/foo")));

	// Check the 'executableFileUrl'...
	CHECK(executableFileUrl(e)->exists());

	// TODO: Test file IO.
} END_TEST
