#include "stdafx.h"
#include "Config.h"

Config config;

void usage(const wchar *name) {
	PLN(L"Usage: " << name << L" [--template <in>] [--out <out>] [--asm <out-asm>] <dir> [--using <use> ...]");
	PLN(L"<dir>     - directories to scan for header files");
	PLN(L"<in>      - input template");
	PLN(L"<out>     - filled in template output");
	PLN(L"<out-asm> - asm-file to output");
	PLN(L"<use>     - using namespace globally");
}

bool parse(int argc, const wchar *argv[]) {
	if (argc < 2)
		return false;

	Path cwd = Path::cwd();

	for (int i = 1; i < argc; i++) {
		if (wcsncmp(argv[i], L"--", 2) == 0) {
			if (i + 1 >= argc) {
				PLN(L"Missing value for " << argv[i]);
				return false;
			}

			if (wcscmp(argv[i], L"--template") == 0) {
				config.src = Path(argv[i+1]).makeAbsolute(cwd);
			} else if (wcscmp(argv[i], L"--out") == 0) {
				config.cppOut = Path(argv[i+1]).makeAbsolute(cwd);
			} else if (wcscmp(argv[i], L"--asm") == 0) {
				config.asmOut = Path(argv[i+1]).makeAbsolute(cwd);
			} else if (wcscmp(argv[i], L"--using") == 0) {
				config.usingDecl.push_back(argv[i+1]);
			} else {
				PLN(L"Unknown option " << argv[i]);
				return false;
			}

			i++;
		} else {
			config.dirs.push_back(Path(argv[i]).makeAbsolute(cwd));
		}
	}

	return true;
}
