#include "stdafx.h"
#include "Compiler/Version.h"

Version *parseVersion(const wchar *str) {
	return parseVersion(new (gEngine()) Str(str));
}

#define CHECK_PARSE(str)						\
	CHECK_EQ(toS(parseVersion(S(str))), String(str))

BEGIN_TEST(VersionTest, Storm) {
	Engine &e = gEngine();

	CHECK_EQ(toS(parseVersion(S("1.2"))), String("1.2.0"));
	CHECK_PARSE("1.2.3");
	CHECK_PARSE("1.2.3-alpha");
	CHECK_PARSE("1.2.3-alpha.beta");
	CHECK_PARSE("1.2.3+git.f00d");
	CHECK_PARSE("1.2.3-alpha+git.f00d");

	Array<Version *> *data = new (e) Array<Version *>();
	data->push(parseVersion(S("1.0.0-alpha")));
	data->push(parseVersion(S("1.0.0-alpha.1")));
	data->push(parseVersion(S("1.0.0-alpha.beta")));
	data->push(parseVersion(S("1.0.0-beta")));
	data->push(parseVersion(S("1.0.0-beta.2")));
	data->push(parseVersion(S("1.0.0-beta.11")));
	data->push(parseVersion(S("1.0.0-rc.1")));
	data->push(parseVersion(S("1.0.0")));
	data->push(parseVersion(S("1.0.1")));
	data->push(parseVersion(S("1.1.0")));
	data->push(parseVersion(S("2.0.0")));

	for (Nat i = 0; i < data->count(); i++) {
		Version &a = *data->at(i);
		for (Nat j = 0; j < i; j++) {
			Version &b = *data->at(j);
			CHECK_GT(a, b);
			CHECK_NEQ(a, b);
		}
		CHECK_EQ(a, a);
		for (Nat j = i + 1; j < data->count(); j++) {
			Version &b = *data->at(j);
			CHECK_LT(a, b);
			CHECK_NEQ(a, b);
		}
	}

} END_TEST
