#pragma once
#include "World.h"

// Parse all files in SrcPos::types and return what we found.
void parseWorld(World &out, const vector<Path> &licenses, const vector<Path> &versions);
