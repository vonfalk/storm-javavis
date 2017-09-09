#pragma once
#include <vector>

/**
 * Import external packages.
 */
struct Import {
	const wchar_t *path;
	const wchar_t *into;
};

/**
 * Parameters from the command line.
 */
class Params {
public:
	Params(int argc, const wchar_t *argv[]);

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
	const wchar_t *modeParam;

	// Second parameter to the mode (might be null).
	const wchar_t *modeParam2;

	// Root path (null if default).
	const wchar_t *root;

	// Import additional packages.
	vector<Import> import;
};
