#include "stdafx.h"
#include "FileReader.h"
#include "Core/Io/Text.h"

namespace storm {

	FileInfo::FileInfo(Url *file, Package *pkg)
		: contents(readAllText(file)), start(contents->begin()), url(file), pkg(pkg) {}

	FileInfo::FileInfo(Str *contents, Str::Iter start, Url *file, Package *pkg)
	: contents(contents), start(start), url(file), pkg(pkg) {}

	FileInfo *FileInfo::next(Str::Iter pos) {
		return new (this) FileInfo(contents, pos, url, pkg);
	}


	FileReader::FileReader(FileInfo *info) : info(info), nextPart(null) {}

	void FileReader::readSyntaxRules() {}

	void FileReader::readSyntaxProductions() {}

	void FileReader::readTypes() {}

	void FileReader::resolveTypes() {}

	void FileReader::readFunctions() {}

	void FileReader::resolveFunctions() {}

	syntax::Rule *FileReader::rootRule() {
		throw new (this) LangDefError(S("This language does not support syntax highlighting."));
	}

	syntax::InfoParser *FileReader::createParser() {
		return new (this) syntax::InfoParser(rootRule());
	}

	FileReader *FileReader::next(ReaderQuery q) {
		if (!nextPart)
			nextPart = createNext(q);
		return nextPart;
	}

	FileReader *FileReader::createNext(ReaderQuery q) {
		return null;
	}

}
