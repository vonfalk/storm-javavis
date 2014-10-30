#pragma once
#include "CppName.h"

/**
 * Describes an exported class.
 */
class Type {
public:
	// Name of the class.
	String name;

	// Superclass (if any, empty otherwise).
	CppName super;

	// Package.
	String package;

	// C++-scope.
	CppName cppName;

};
