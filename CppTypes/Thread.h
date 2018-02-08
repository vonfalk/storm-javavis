#pragma once
#include "CppName.h"
#include "SrcPos.h"
#include "Auto.h"

/**
 * Describe a thread declared in C++ (using STORM_THREAD(Foo)).
 */
class Thread : public Refcount {
public:
	Thread(const CppName &name, const String &pkg, const SrcPos &pos, const String &doc, bool external);

	// Name.
	CppName name;

	// Package.
	String pkg;

	// Position.
	SrcPos pos;

	// Documentation.
	String doc;

	// Id.
	nat id;

	// External?
	bool external;
};
