#include "stdafx.h"
#include "Config.h"
#include "SrcPos.h"
#include "World.h"

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

// Find the latest last-modified time for any files.
Timestamp lastModified(const vector<Path> &files) {
	Timestamp last = files[0].mTime();
	for (nat i = 1; i < files.size(); i++)
		last = max(last, files[i].mTime());
	return last;
}

// Update a timestamp if 'file' exists.
Timestamp max(const Timestamp &t, const Path &file) {
	if (file.exists())
		return max(t, file.mTime());
	else
		return t;
}

// Do we need to update the file?
bool oldFile(const Timestamp &input, const Path &file) {
	if (file.isEmpty())
		return false;

	if (!file.exists())
		return true;

	return file.mTime() < input;
}

int _tmain(int argc, const wchar *argv[]) {
	if (!parse(argc, argv)) {
		usage(argv[0]);
		return 1;
	}

	Timestamp start;

	// Put the files in our global, so we can use SrcPos later on.
	SrcPos::files = findHeaders(config.dir);
	if (SrcPos::files.empty()) {
		PLN("No header files found.");
		return 1;
	}

	Timestamp modified = lastModified(SrcPos::files);
	modified = max(modified, config.src);

	bool update = oldFile(modified, config.cppOut) || oldFile(modified, config.asmOut);

	if (update) {
		try {
			World world = parseWorld();
		} catch (const Exception &e) {
			PVAR(e.what());
			PLN("FAILED");
			return 1;
		}
	}

	Timestamp end;
	PLN(L"Total time: " << (end - start));

	return 0;
}
