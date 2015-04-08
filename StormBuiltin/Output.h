#pragma once
#include "Type.h"
#include "Header.h"

/**
 * Generate and write output.
 */

// Generate the list of types.
String typeList(const Types &t, const vector<Thread> &threads);

// Generate functions for each type.
String typeFunctions(const Types &t);

// Generate the vtable code (asm).
String vtableCode(const Types &t);

// Generate list of functions.
String functionList(const vector<Header *> &headers, const Types &t, const vector<Thread> &threads);

// Generate list of variables.
String variableList(const vector<Header *> &headers, const Types &t);

// Generate list of includes
String headerList(const vector<Header *> &headers, const Path &root);

// Generate list of threads.
String threadList(const vector<Thread> &threads);

