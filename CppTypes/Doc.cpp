#include "stdafx.h"
#include "Doc.h"
#include "Config.h"
#include "World.h"

Doc::Doc(const Comment &data) : v(data.str()), fileId(0) {}

Doc::Doc(const String &data) : v(data), fileId(0) {}

nat Doc::id(World &world) {
	// Already generated?
	if (fileId)
		return fileId;

	if (config.docOut.isEmpty()) {
		// We do not want to generate documentation at all.
		return 0;
	}

	world.documentation.push_back(capture(this));
	fileId = nat(world.documentation.size());

	return fileId;
}

wostream &operator <<(wostream &to, const Doc &doc) {
	return to << doc.v;
}
