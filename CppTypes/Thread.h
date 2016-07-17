#pragma once
#include "CppName.h"
#include "SrcPos.h"
#include "Auto.h"

/**
 * Describe a thread declared in C++ (using STORM_THREAD(Foo)).
 */
class Thread : public Refcount {
public:
	Thread(const CppName &name, const SrcPos &pos);

	// Name.
	CppName name;

	// Position.
	SrcPos pos;

	// Id.
	nat id;
};
