#include "stdafx.h"
#include "Utils/Path.h"
#include "Function.h"

using namespace stormbuiltin;

void findFunctions(const Path &p) {
	vector<Path> children = p.children();
	for (nat i = 0; i < children.size(); i++) {
		Path &c = children[i];

		if (c.isDir())
			findFunctions(c);
		else if (c.hasExt(L"h"))
			parseFile(c);
	}
}

int _tmain(int argc, _TCHAR* argv[]) {
	initDebug();

	findFunctions(Path::executable() + Path(L"../Storm/"));

	return 0;
}

