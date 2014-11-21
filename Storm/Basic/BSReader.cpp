#include "stdafx.h"
#include "BSReader.h"
#include "Parser.h"
#include "SyntaxTransform.h"
#include "BSIncludes.h"
#include "BSContents.h"
#include "Utils/FileStream.h"

namespace storm {

	bs::Reader::Reader(Auto<PkgFiles> files, Auto<Package> pkg) : FilesReader(files, pkg) {}

	FileReader *bs::Reader::createFile(const Path &path) {
		return CREATE(bs::File, this, path, owner);
	}


	bs::File::File(const Path &path, Auto<Package> owner) : FileReader(path, owner) {
		fileContents = readTextFile(path);
		readIncludes();
	}

	bs::File::~File() {}

	void bs::File::readTypes() {
		readContents();
		for (nat i = 0; i < contents->types.size(); i++) {
			package->add(contents->types[i]);
		}
	}

	void bs::File::readFunctions() {
		readContents();
		Scope s = scope();
		for (nat i = 0; i < contents->functions.size(); i++) {
			package->add(contents->functions[i]->asFunction(s));
		}
	}

	Scope bs::File::scope() {
		Scope scope(package.borrow());
		for (nat i = 0; i < includes.size(); i++) {
			scope.extra.push_back(includes[i]);
		}
		return scope;
	}

	void bs::File::readContents() {
		if (contents)
			return;

		Parser parser(syntax, fileContents);
		parser.parse(L"File", headerSize);
		if (parser.hasError())
			throw parser.error(file);

		Auto<Object> includes = parser.transform(package->engine, file);
		contents = includes.as<Contents>();

		if (!contents) {
			throw InternalError(L"Invalid type returned from rules for .bs");
		}
	}

	void bs::File::readIncludes() {
		Auto<Object> includes;

		syntax.add(*syntaxPackage());
		syntax.add(*package);

		Parser parser(syntax, fileContents);
		headerSize = parser.parse(L"Includes");
		if (headerSize == parser.NO_MATCH)
			throw parser.error(file);

		includes = parser.transform(package->engine, file);

		if (Includes *inc = as<Includes>(includes.borrow())) {
			setIncludes(inc->names);
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
