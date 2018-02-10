#include "stdafx.h"
#include "Doc.h"
#include "Config.h"

Doc::Doc(const Comment &data) : v(data.str()), fileId(0) {}

Doc::Doc(const String &data) : v(data), fileId(0) {}

nat Doc::id() const {
	if (!config.docOut.isEmpty())
		assert(fileId > 0, L"This comment has not been written to a documentation file yet!");
	return fileId;
}

wostream &operator <<(wostream &to, const Doc &doc) {
	return to << doc.v;
}
