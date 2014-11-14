#include "stdafx.h"
#include "BSReader.h"
#include "Parser.h"
#include "SyntaxTransform.h"
#include "BSIncludes.h"
#include "Utils/FileStream.h"

namespace storm {

	bs::Reader::Reader(Auto<PkgFiles> files) : FilesReader(files) {}

	FileReader *bs::Reader::createFile(const Path &path) {
		return CREATE(bs::File, this, path, owner);
	}


	bs::File::File(const Path &path, Package *owner) : FileReader(path, owner) {
		contents = readTextFile(path);
		readIncludes();
	}

	bs::File::~File() {}

	void bs::File::readTypes() {}

	void bs::File::readIncludes() {
		SyntaxNode *rootNode = null;
		Auto<Object> includes;

		try {
			syntax.add(*syntaxPackage());
			syntax.add(*package);
			Parser parser(syntax, contents);
			headerSize = parser.parse(L"Includes");
			if (headerSize == parser.NO_MATCH)
				throw parser.error(file);

			rootNode = parser.tree();
			includes = transform(package->engine, syntax, *rootNode);

			if (Includes *inc = as<Includes>(includes.borrow())) {
				setIncludes(inc->names);
			}

			delete rootNode;
			includes->release();
		} catch (...) {
			delete rootNode;
			includes->release();
			throw;
		}
	}

	void bs::File::setIncludes(const vector<Name> &inc) {
		for (nat i = 0; i < inc.size(); i++) {
			Package *p = package->engine.package(inc[i]);
			if (!p)
				throw SyntaxError(SrcPos(file, 0), L"Unknown package " + ::toS(inc[i]));
			includes.push_back(p);
		}
	}

}
