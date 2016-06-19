#include "stdafx.h"
#include "Config.h"

void findHeaders(const Path &in, vector<Path> &out) {
	vector<Path> c = in.children();
	for (nat i = 0; i < c.size(); i++) {
		if (c[i].isDir()) {
			findHeaders(c[i], out);
		} else if (c[i].hasExt(L"h")) {
			out.push_back(c[i]);
		}
	}
}

// Find headers to process.
vector<Path> findHeaders(const Path &in) {
	vector<Path> result;
	findHeaders(in, result);
	return result;
}

int _tmain(int argc, const wchar *argv[]) {
	if (!parse(argc, argv)) {
		usage(argv[0]);
		return 1;
	}

	Timestamp start;

	vector<Path> files = findHeaders(config.dir);
	PVAR(files);

	Timestamp end;
	PLN(L"Total time: " << (end - start));

	return 0;
}
