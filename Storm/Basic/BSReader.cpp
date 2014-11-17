#include "stdafx.h"
#include "BSReader.h"
#include "Parser.h"
#include "SyntaxTransform.h"
#include "BSIncludes.h"
#include "Utils/FileStream.h"

namespace storm {

	bs::Reader::Reader(Auto<PkgFiles> files, Auto<Package> pkg) : FilesReader(files, pkg) {}

	FileReader *bs::Reader::createFile(const Path &path) {
		return CREATE(bs::File, this, path, owner);
	}


	bs::File::File(const Path &path, Auto<Package> owner) : FileReader(path, owner) {
		contents = readTextFile(path);
		readIncludes();
	}

	bs::File::~File() {}

	void bs::File::readTypes() {
		Parser parser(syntax, contents);
		parser.parse(L"File", headerSize);
		if (parser.hasError())
			throw parser.error(file);

		SyntaxNode *root = parser.tree(file);
		try {
			Auto<Object> includes = transform(package->engine, syntax, *root);
			delete root;
		} catch (...) {
			delete root;
			throw;
		}
	}

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

			rootNode = parser.tree(file);
			includes = transform(package->engine, syntax, *rootNode);

			if (Includes *inc = as<Includes>(includes.borrow())) {
				setIncludes(inc->names);
			}

			delete rootNode;
		} catch (...) {
			delete rootNode;
			throw;
		}
	}

	void bs::File::setIncludes(const vector<Name> &inc) {
		for (nat i = 0; i < inc.size(); i++) {
			Package *p = package->engine.package(inc[i]);
			if (!p)
				throw SyntaxError(SrcPos(file, 0), L"Unknown package " + ::toS(inc[i]));
			includes.push_back(p);
			syntax.add(*p);
		}
	}

}
