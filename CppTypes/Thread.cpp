#include "stdafx.h"
#include "Thread.h"

Thread::Thread(const CppName &name, const String &pkg, const SrcPos &pos, const Auto<Doc> &doc, bool external)
	: name(name), pkg(pkg), pos(pos), doc(doc), external(external) {}

