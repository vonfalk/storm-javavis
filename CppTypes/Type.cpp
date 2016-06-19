#include "stdafx.h"
#include "Type.h"

Type::Type(const CppName &name, const SrcPos &pos) : name(name), parent(L""), pos(pos) {}

