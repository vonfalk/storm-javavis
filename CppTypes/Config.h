#pragma once
#include "Utils/Path.h"

/**
 * Configuration for the current execution.
 */
struct Config {
	Config();

	// Directories to scan for header files.
	vector<Path> dirs;

	// Template src.
	Path cppSrc, asmSrc;

	// Output to.
	Path cppOut, asmOut;

	// Do we have asm source?
	bool genAsm;

	// Using declarations.
	vector<String> usingDecl;
};

extern Config config;

void usage(const wchar *name);
bool parse(int argc, const wchar *argv[]);
