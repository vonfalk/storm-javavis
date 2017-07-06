#include "stdafx.h"
#include "SrcPos.h"

SrcPos::SrcPos() : fileId(invalid), line(0), col(0) {}

SrcPos::SrcPos(nat fileId, nat line, nat col) : fileId(fileId), line(line), col(col) {}

vector<Path> SrcPos::files;

nat SrcPos::firstExport = 0;

wostream &operator <<(wostream &to, const SrcPos &pos) {
	if (pos.fileId == SrcPos::invalid) {
		to << L"<unknown location>";
	} else {
#ifdef VISUAL_STUDIO
		to << SrcPos::files[pos.fileId] << L"(" << (pos.line+1) << L"," << (pos.col+1) << L")";
#else
		to << SrcPos::files[pos.fileId] << L":" << (pos.line+1) << L":" << (pos.col+1);
#endif
	}
	return to;
}
