#pragma once
#include "Utils/Path.h"

/**
 * Configuration for the current execution.
 */
struct Config {
	Config();

	// Directories to scan for header files.
	vector<Path> dirs;

	// Directories to scan for header files containing types used here, but not provided by this unit.
	vector<Path> useDirs;

	// Template src.
	Path cppSrc, asmSrc;

	// Output to.
	Path cppOut, asmOut, docOut;

	// Do we have asm source?
	bool genAsm;

	// Is this the compiler?
	bool compiler;

	// Using declarations.
	vector<String> usingDecl;
};

extern Config config;

void usage(const wchar_t *name);
bool parse(int argc, const wchar_t *argv[]);
