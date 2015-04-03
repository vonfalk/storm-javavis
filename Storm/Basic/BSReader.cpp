#include "stdafx.h"
#include "BSReader.h"
#include "Parser.h"
#include "SyntaxTransform.h"
#include "BSIncludes.h"
#include "BSContents.h"
#include "BSClass.h"
#include "Io/Text.h"


namespace storm {

	bs::Reader::Reader(Par<PkgFiles> files, Par<Package> pkg) : FilesReader(files, pkg) {}

	FileReader *bs::Reader::createFile(Par<Url> path) {
		return CREATE(bs::File, this, path, owner);
	}


	bs::File::File(Par<Url> path, Par<Package> owner)
		: FileReader(path, owner), scopeLookup(CREATE(BSScope, engine(), path)), scope(owner, scopeLookup) {

		Auto<Str> c = readAllText(path);
		fileContents = c->v;
		readIncludes();
	}

	bs::File::~File() {}

	void bs::File::readTypes() {
		readContents();
		for (nat i = 0; i < contents->types.size(); i++) {
			package->add(contents->types[i].borrow());
		}

		for (nat i = 0; i < contents->threads.size(); i++) {
			package->add(contents->threads[i].borrow());
		}
	}

	void bs::File::resolveTypes() {
		readContents();
		for (nat i = 0; i < contents->types.size(); i++) {
			if (Class *c = as<Class>(contents->types[i].borrow()))
				c->lookupTypes();
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

		Auto<Object> includes = parser.transform(package->engine());
		contents = includes.expect<Contents>(package->engine(), L"While evaluating File");
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

		includes = parser.transform(package->engine());

		if (Includes *inc = as<Includes>(includes.borrow())) {
			setIncludes(inc->names);
		}
	}

	void bs::File::setIncludes(const vector<Auto<TypeName> > &inc) {
		for (nat i = 0; i < inc.size(); i++) {
			Auto<Name> name = inc[i]->toName(scope);
			Package *p = package->engine().package(name);
			if (!p)
				throw SyntaxError(SrcPos(file, 0), L"Unknown package " + ::toS(inc[i]));

			addInclude(scope, p);
		}

		addSyntax(scope, syntax);
	}

}
