#include "stdafx.h"
#include "Config.h"
#include "SrcPos.h"
#include "World.h"
#include "Parse.h"
#include "Write.h"
#include "Output.h"

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
vector<Path> findHeaders(const vector<Path> &in) {
	vector<Path> result;
	for (nat i = 0; i < in.size(); i++)
		findHeaders(in[i], result);
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
	SrcPos::files = findHeaders(config.dirs);
	if (SrcPos::files.empty()) {
		PLN("No header files found.");
		return 1;
	}

	Timestamp modified = lastModified(SrcPos::files);
	modified = max(modified, config.src);
	{
		Path me(argv[0]);
		if (me.exists())
			modified = max(modified, me.mTime());
	}

	bool update = oldFile(modified, config.cppOut);
	// TODO: Enable when we output asm properly!
	// update |= oldFile(modified, config.asmOut);

	if (update) {
		try {
			World world = parseWorld();

			// TODO: More!
			world.usingDecl.push_back(CppName(L"storm"));
			for (nat i = 0; i < config.usingDecl.size(); i++)
				world.usingDecl.push_back(CppName(config.usingDecl[i]));

			world.prepare();
			generateFile(config.src, config.cppOut, genMap(), world);
		} catch (const Exception &e) {
			PLN(e);
			// PLN(e.what());
			// PVAR(e);
			PLN("FAILED");
			return 1;
		}
	}

	Timestamp end;
	PLN(L"Total time: " << (end - start));

	return 0;
}
