#include "stdafx.h"
#include "DbgHelper.h"

#ifdef WINDOWS

namespace code {

	static bool init(HANDLE process) {
		SymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);
		if (SymInitialize(process, NULL, TRUE))
			return true;

		WARNING(L"Failed to initialize DbgHelp: " << GetLastError());
		return false;
	}

	DbgHelp::DbgHelp() : process(GetCurrentProcess()), initialized(init(process)) {}

	DbgHelp::~DbgHelp() {
		if (initialized)
			SymCleanup(process);
	}

	DbgHelp &dbgHelp() {
		static DbgHelp r;
		return r;
	}

}

#endif
