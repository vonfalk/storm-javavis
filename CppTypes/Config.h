#pragma once
#include "Utils/Path.h"

/**
 * Configuration for the current execution.
 */
struct Config {
	// Directory to scan for header files.
	Path dir;

	// Template src.
	Path src;

	// Output to.
	Path cppOut, asmOut;
};

extern Config config;

void usage(const wchar *name);
bool parse(int argc, const wchar *argv[]);
