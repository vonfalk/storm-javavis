#pragma once

#include "Utils/Path.h"

struct FileData {
	String typeList;
	String typeFunctions;
	String vtableCode;
	String functionList;
	String headerList;
	String threadList;
};

void update(const Path &inFile, const Path &outFile, const Path &asmFile, const FileData &data);
