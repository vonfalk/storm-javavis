#pragma once
#include "CppName.h"
#include "SrcPos.h"
#include "Auto.h"
#include "Doc.h"

/**
 * Describe a thread declared in C++ (using STORM_THREAD(Foo)).
 */
class Thread : public Refcount {
public:
	Thread(const CppName &name, const String &pkg, const SrcPos &pos, const Auto<Doc> &doc, bool external);

	// Name.
	CppName name;

	// Package.
	String pkg;

	// Position.
	SrcPos pos;

	// Documentation.
	Auto<Doc> doc;

	// Id.
	nat id;

	// External?
	bool external;
};
