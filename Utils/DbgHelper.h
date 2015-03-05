#pragma once
#include "Platform.h"

#ifdef WINDOWS

// We want wchars in the lib!
#define DBGHELP_TRANSLATE_TCHAR
#include <Dbghelp.h>
#pragma comment (lib, "Dbghelp.lib")

/**
 * Helpers for using DbgHelp on Windows.
 */
class DbgHelp : NoCopy {
	friend DbgHelp &dbgHelp();

	// Ctor.
	DbgHelp();

public:
	// Handle to our process.
	const HANDLE process;

	// Initialized?
	const bool initialized;

	// Dtor.
	~DbgHelp();
};

// Static instance (lazy).
DbgHelp &dbgHelp();

#endif
