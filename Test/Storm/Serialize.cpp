#include "stdafx.h"
#include "Fn.h"

BEGIN_TEST(Serialize, BS) {

	// Simple serialization.
	CHECK(runFn<Bool>(S("test.bs.simpleSerialization")));

	// Multiple times in the same stream.
	CHECK(runFn<Bool>(S("test.bs.multipleSerialization")));

	// Shuffled output (simulating altering the order of members from when serialization was made).
	CHECK(runFn<Bool>(S("test.bs.shuffledSerialization")));

	// Primitives.
	CHECK(runFn<Bool>(S("test.bs.primitiveSerialization")));

	// Reading/writing different formal types.
	CHECK(runFn<Bool>(S("test.bs.typeDiffSerialization")));

	// Serialization of containers.
	CHECK(runFn<Bool>(S("test.bs.arraySerialization")));
	CHECK(runFn<Bool>(S("test.bs.mapSerialization")));
	CHECK(runFn<Bool>(S("test.bs.setSerialization")));
	CHECK(runFn<Bool>(S("test.bs.maybeSerialization")));

	// Custom serialization.
	CHECK(runFn<Bool>(S("test.bs.customSerialization")));

} END_TEST


BEGIN_TESTX(SerializePerf, BS) {
	runFn<void>(S("test.bs.serializationPerf"));
} END_TEST
