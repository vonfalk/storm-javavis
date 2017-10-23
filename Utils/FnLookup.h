#pragma once
#include "StackTrace.h"
#include "Lock.h"

/**
 * Lookup function names from their addresses.
 *
 * Default implementation: only knows about C++ generated functions if
 *  debug information is present.
 */
class FnLookup : NoCopy {
public:
	// Format a function call. Returns 'true' if an entry was found and emitted to the wostream,
	// otherwise 'false'.
	virtual bool format(std::wostream &to, const StackFrame &frame) const = 0;
};

/**
 * Basic lookup, using debug information if available.
 */
class CppLookup : public FnLookup {
public:
	// Format a function call.
	virtual bool format(std::wostream &to, const StackFrame &frame) const;
};



/**
 * Keep track of a global set of registered lookup objects.
 */
class FnLookups : NoCopy {
public:
	// Register a new FnLookup. Returns its ID, or -1 if a lookup of the same type was already registered.
	int attach(const FnLookup &lookup);

	// Detach a previously registered FnLookup.
	void detach(int id);

	// Format a StackFrame according to the lookups in here.
	void format(wostream &to, const StackFrame &frame);

private:
	FnLookups();
	friend FnLookups &fnLookups();

	// All currently registered lookup objects. May contain null entries.
	vector<const FnLookup *> data;

	// Protect the data here with a lock.
	util::Lock lock;
};

// Get the single instance of FnLookups.
FnLookups &fnLookups();


/**
 * Helper class to register and de-register Lookup objects.
 */
template <class T>
class RegisterLookup {
public:
	RegisterLookup() {
		id = fnLookups().attach(lookup);
	}

	~RegisterLookup() {
		fnLookups().detach(id);
	}

private:
	T lookup;
	int id;
};
