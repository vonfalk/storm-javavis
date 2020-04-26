#include "stdafx.h"
#include "SrcPos.h"

SrcPos::SrcPos() : fileId(invalid), pos(0) {}

SrcPos::SrcPos(nat fileId, nat pos) : fileId(fileId), pos(pos) {}

vector<Path> SrcPos::files;

nat SrcPos::firstExport = 0;

wostream &operator <<(wostream &to, const SrcPos &pos) {
	if (pos.fileId == SrcPos::invalid) {
		to << L"<unknown location>";
	} else {
		to << "@" << SrcPos::files[pos.fileId] << L"(" << pos.pos << L")";
	}
	return to;
}
