// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>

#include "Utils/Utils.h"
#include "Code/Code.h"

#include "Utils/ConUtils.h"

// Include most of Code:s public API.
#include "Code/Arena.h"
#include "Code/Code.h"
#include "Code/Arena.h"
#include "Code/Binary.h"
#include "Code/Instruction.h"
#include "Code/Listing.h"
#include "Code/MachineCode.h"
#include "Code/Exception.h"
#include "Code/Function.h"

using namespace std;
using namespace code;

// Test code!
#include "Test/Test.h"

// Call function, checking extra registers.
int callFn(const void *fnPtr, int p);
int64 callFn(const void *fnPtr, int64 p);
