#pragma once
#include "Utils/Path.h"

class World;

// Function which generates data for the file.
typedef void (*GenerateFn)(wostream &to, World &world);

// Description of all markers in the template we want to replace.
typedef map<String, GenerateFn> GenerateMap;

// Generate a file from a template.
void generateFile(const Path &src, const Path &dest, const GenerateMap &actions, World &world);

// Generate the documentation file.
void generateDoc(const Path &out, World &world);
