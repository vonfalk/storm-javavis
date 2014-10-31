#pragma once

#include "Utils/Path.h"

struct FileData {
	String typeList;
	String typeFunctions;
	String vtableCode;
	String functionList;
	String headerList;
};

void update(const Path &outFile, const Path &asmFile, const FileData &data);
