#include "stdafx.h"
#include "Fn.h"

BEGIN_TEST(Serialize, BS) {

	// Simple serialization.
	CHECK(runFn<Bool>(S("tests.bs.simpleSerialization")));

	// Multiple times in the same stream.
	CHECK(runFn<Bool>(S("tests.bs.multipleSerialization")));

	// Shuffled output (simulating altering the order of members from when serialization was made).
	CHECK(runFn<Bool>(S("tests.bs.shuffledSerialization")));

	// Primitives.
	CHECK(runFn<Bool>(S("tests.bs.primitiveSerialization")));

	// Reading/writing different formal types.
	CHECK(runFn<Bool>(S("tests.bs.typeDiffSerialization")));

	// Serialization of containers.
	CHECK(runFn<Bool>(S("tests.bs.arraySerialization")));
	CHECK(runFn<Bool>(S("tests.bs.mapSerialization")));
	CHECK(runFn<Bool>(S("tests.bs.setSerialization")));
	CHECK(runFn<Bool>(S("tests.bs.maybeSerialization")));

	// Custom serialization.
	CHECK(runFn<Bool>(S("tests.bs.customSerialization")));

	// Serialization of Url.
	CHECK(runFn<Bool>(S("tests.bs.urlSerialization")));

	// Serialization of Version.
	CHECK(runFn<Bool>(S("tests.bs.versionSerialization")));

} END_TEST


BEGIN_TESTX(SerializePerf, BS) {
	runFn<void>(S("tests.bs.serializationPerf"));
} END_TEST
