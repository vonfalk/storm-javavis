#pragma once
#include "StackInfo.h"
#include "CppInfo.h"


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

	// Translate a function pointer into base pointer and offset. Returns an ID of the backend that
	// captured the data.
	int translate(void *ip, void *&fnBase, int &offset);

	// Format a function call. Pass the 'id' returned from 'tranlsate'.
	void format(GenericOutput &to, int id, void *fnBase, int offset);

private:
	StackInfoSet();

	friend StackInfoSet &stackInfo();

	// All currently registered lookup objects. May contain null entries.
	vector<const StackInfo *> data;

	// Protect the data here with a lock.
	util::Lock lock;

	// Fallback.
	CppInfo cppInfo;
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

	T *operator ->() {
		return &lookup;
	}

private:
	T lookup;
	int id;
};
