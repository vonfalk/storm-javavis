#pragma once
#include "Type.h"

/**
 * Generate and write output.
 */

// Generate the list of types.
String typeList(const Types &t);

// Generate functions for each type.
String typeFunctions(const Types &t);

// Generate the vtable code (asm).
String vtableCode(const Types &t);

