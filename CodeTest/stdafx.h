#pragma once

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
#include "Code/FnParams.h"

using namespace std;
using namespace code;
using namespace os;

// Test code!
#include "Test/Lib/Test.h"

// Call function, checking extra registers.
int callFn(const void *fnPtr, int p);
int64 callFn(const void *fnPtr, int64 p);

