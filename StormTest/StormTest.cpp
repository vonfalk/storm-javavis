#include "stdafx.h"

#include "Test/TestMgr.h"

int _tmain(int argc, _TCHAR* argv[])
{
	initDebug();

	Timestamp start;

	Tests::run();

	Timestamp end;
	PLN("Total time: " << (end - start));

	return 0;
}

