#pragma once
#include "Utils/Path.h"

/**
 * Configuration for the current execution.
 */
struct Config {
	// Directories to scan for header files.
	vector<Path> dirs;

	// Template src.
	Path src;

	// Output to.
	Path cppOut, asmOut;
};

extern Config config;

void usage(const wchar *name);
bool parse(int argc, const wchar *argv[]);
