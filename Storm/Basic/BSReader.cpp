#include "stdafx.h"
#include "BSReader.h"
#include "Parser.h"
#include "SyntaxTransform.h"
#include "BSIncludes.h"
#include "BSContents.h"
#include "Utils/FileStream.h"

namespace storm {

	bs::Reader::Reader(Par<PkgFiles> files, Par<Package> pkg) : FilesReader(files, pkg) {}

	FileReader *bs::Reader::createFile(const Path &path) {
		return CREATE(bs::File, this, path, owner);
	}


	bs::File::File(const Path &path, Par<Package> owner)
		: FileReader(path, owner), scopeLookup(CREATE(BSScope, engine(), path)), scope(owner, scopeLookup) {

		fileContents = readTextFile(path);
		readIncludes();
	}

	bs::File::~File() {}

	void bs::File::readTypes() {
		readContents();
		for (nat i = 0; i < contents->types.size(); i++) {
			package->add(contents->types[i].borrow());
		}
	}

	void bs::File::readFunctions() {
		readContents();
		for (nat i = 0; i < contents->functions.size(); i++) {
			package->add(steal(contents->functions[i]->asFunction(scope)));
		}
	}

	void bs::File::readContents() {
		if (contents)
			return;

		Parser parser(syntax, fileContents, file);
		parser.parse(L"File", headerSize);
		if (parser.hasError())
			throw parser.error();

		Auto<Object> includes = parser.transform(package->engine);
		contents = includes.expect<Contents>(package->engine, L"While evaluating File");
		contents->setScope(scope);
	}

	void bs::File::readIncludes() {
		Auto<Object> includes;

		syntax.add(*syntaxPackage());
		syntax.add(*package);

		Parser parser(syntax, fileContents, file);
		headerSize = parser.parse(L"Includes");
		if (headerSize == parser.NO_MATCH)
			throw parser.error();

		includes = parser.transform(package->engine);

		if (Includes *inc = as<Includes>(includes.borrow())) {
			setIncludes(inc->names);
		}
	}

	void bs::File::setIncludes(const vector<Name> &inc) {
		for (nat i = 0; i < inc.size(); i++) {
			Package *p = package->engine.package(inc[i]);
			if (!p)
				throw SyntaxError(SrcPos(file, 0), L"Unknown package " + ::toS(inc[i]));

			addInclude(scope, p);
		}

		addSyntax(scope, syntax);
	}

}
