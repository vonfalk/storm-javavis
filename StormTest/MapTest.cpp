#include "stdafx.h"
#include "Test/Test.h"

BEGIN_TEST_(MapTest) {
	Engine &e = *gEngine;

	typedef Map<Auto<Str>, Int> SIMap;
	Auto<SIMap> map = CREATE(SIMap, e);
	PLN("Map created!");

} END_TEST
