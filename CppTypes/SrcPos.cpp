#include "stdafx.h"
#include "SrcPos.h"

SrcPos::SrcPos() : fileId(invalid), offset(0) {}

SrcPos::SrcPos(nat fileId, nat offset) : fileId(fileId), offset(offset) {}

vector<Path> SrcPos::files;

wostream &operator <<(wostream &to, const SrcPos &pos) {
	if (pos.fileId == SrcPos::invalid)
		to << L"<unknown location>";
	else
		to << SrcPos::files[pos.fileId] << L"(+" << pos.offset << L")";
	return to;
}
