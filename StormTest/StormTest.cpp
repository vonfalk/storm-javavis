#include "stdafx.h"

#include "Test/TestMgr.h"


int _tmain(int argc, _TCHAR* argv[])
{
	initCrt();

	Tests::run();

	return 0;
}

