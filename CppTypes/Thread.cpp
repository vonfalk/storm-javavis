#include "stdafx.h"
#include "Thread.h"

Thread::Thread(const CppName &name, const String &pkg, const SrcPos &pos, bool external)
	: name(name), pkg(pkg), pos(pos), external(external) {}

