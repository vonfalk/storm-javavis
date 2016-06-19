#include "stdafx.h"
#include "Config.h"

Config config;

void usage(const wchar *name) {
	PLN(L"Usage: " << name << L" <dir> [--template <in>] [--out <out>] [--asm <out-asm>]");
	PLN(L"<dir>     - directory to scan for header files");
	PLN(L"<in>      - input template");
	PLN(L"<out>     - filled in template output");
	PLN(L"<out-asm> - asm-file to output");
}

bool parse(int argc, const wchar *argv[]) {
	if (argc < 2)
		return false;

	Path cwd = Path::cwd();

	config.dir = Path(argv[1]).makeAbsolute(cwd);

	for (int i = 2; i < argc; i += 2) {
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
		} else {
			PLN(L"Unknown option " << argv[i]);
			return false;
		}
	}

	return true;
}
