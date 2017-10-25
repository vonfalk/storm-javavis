#pragma once
#include "StackTrace.h"
#include "Lock.h"

/**
 * Pluggable interface for collecting and showing information on the stack.
 */
class StackInfo : NoCopy {
public:
	// Called when the system allocates an array of StackFrame structures so that a backend can keep
	// any data referred by the trace alive until the trace is not needed anymore. The function is
	// called after the stack trace is populated.
	virtual void alloc(StackFrame *frames, nat count) const;
	virtual void free(StackFrame *frames, nat count) const;

	// Called while collecting a stack trace to collect any additional data that might be required
	// when displaying the stack trace.
	virtual void collect(StackFrame &to, void *frame) const;

	// Format a function call. Returns 'true' if an entry was found and emitted to the wostream,
	// otherwise 'false'.
	virtual bool format(std::wostream &to, const StackFrame &frame) const;
};


/**
 * Keep track of a global set of registered lookup objects.
 */
class StackInfoSet : NoCopy {
public:
	// Register a new FnLookup. Returns its ID, or -1 if a lookup of the same type was already registered.
	int attach(const StackInfo &lookup);

	// Detach a previously registered FnLookup.
	void detach(int id);

	// Called when the system allocates an array of StackFrame structures so that a backend can keep
	// any data referred by the trace alive until the trace is not needed anymore. The function is
	// called after the stack trace is populated.
	void alloc(StackFrame *frames, nat count);
	void free(StackFrame *frames, nat count);

	// Called while collecting a stack trace to collect any additional data that might be required
	// when displaying the stack trace.
	void collect(StackFrame &to, void *frame);

	// Format a function call. Returns 'true' if an entry was found and emitted to the wostream,
	// otherwise 'false'.
	void format(std::wostream &to, const StackFrame &frame);

private:
	StackInfoSet();
	friend StackInfoSet &stackInfo();

	// All currently registered lookup objects. May contain null entries.
	vector<const StackInfo *> data;

	// Protect the data here with a lock.
	util::Lock lock;
};

// Get the single instance of StackInfoSet.
StackInfoSet &stackInfo();


/**
 * Helper class to register and de-register Lookup objects.
 */
template <class T>
class RegisterInfo {
public:
	RegisterInfo() {
		id = stackInfo().attach(lookup);
	}

	~RegisterInfo() {
		stackInfo().detach(id);
	}

private:
	T lookup;
	int id;
};
