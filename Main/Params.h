#pragma once
#include <vector>

/**
 * Import external packages.
 */
struct Import {
	const wchar *path;
	const wchar *into;
};

/**
 * Parameters from the command line.
 */
class Params {
public:
	Params(int argc, const wchar *argv[]);

	/**
	 * What mode shall we run in?
	 */
	enum Mode {
		modeHelp,
		modeRepl,
		modeFunction,
		modeServer,
		modeError,
	};

	// Mode.
	Mode mode;

	// Parameter to the mode (might be null).
	const wchar *modeParam;

	// Second parameter to the mode (might be null).
	const wchar *modeParam2;

	// Root path (null if default).
	const wchar *root;

	// Import additional packages.
	vector<Import> import;
};
