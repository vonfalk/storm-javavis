#include "stdafx.h"
#include "BSReader.h"
#include "BSTemplate.h"
#include "Parser.h"
#include "SyntaxTransform.h"
#include "BSContents.h"
#include "BSClass.h"
#include "Engine.h"
#include "SyntaxEnv.h"
#include "Shared/Io/Text.h"


namespace storm {

	bs::Reader::Reader(Par<PkgFiles> files, Par<Package> pkg) : FilesReader(files, pkg) {}

	FileReader *bs::Reader::createFile(Par<Url> path) {
		return CREATE(bs::File, this, path, pkg);
	}


	bs::File::File(Par<Url> path, Par<Package> owner)
		: FileReader(path, owner), scopeLookup(CREATE(BSScope, engine(), path)), scope(owner, scopeLookup) {

		syntax = CREATE(SyntaxSet, this);
		fileContents = readAllText(path);
	}

	bs::File::~File() {}

	void bs::File::readTypes() {
		readContents();
		for (nat i = 0; i < contents->types->count(); i++) {
			pkg->add(contents->types->at(i).borrow());
		}

		for (nat i = 0; i < contents->threads->count(); i++) {
			pkg->add(contents->threads->at(i).borrow());
		}
	}

	void bs::File::resolveTypes() {
		readContents();
		for (nat i = 0; i < contents->types->count(); i++) {
			if (Class *c = as<Class>(contents->types->at(i).borrow()))
				c->lookupTypes();
		}
	}

	void bs::File::readFunctions() {
		readContents();
		for (nat i = 0; i < contents->functions->count(); i++) {
			pkg->add(steal(contents->functions->at(i)->createFn()));
		}

		typedef MAP_PP(Str, TemplateAdapter)::Iter Iter;
		for (Iter i = contents->templates->begin(), end = contents->templates->end(); i != end; i++) {
			pkg->add(i.val());
		}
	}

	void bs::File::readContents() {
		if (contents)
			return;

		readIncludes();

		Auto<Parser> parser = CREATE(Parser, this, syntax, fileContents, file);
		parser->parse(L"File", headerSize);
		if (parser->hasError())
			throw parser->error();

		Auto<SyntaxEnv> env = CREATE(SyntaxEnv, this, scope);

		Auto<Object> includes = parser->transform(vector<Object *>(1, env.borrow()));
		contents = includes.expect<Contents>(pkg->engine(), L"While evaluating File");
	}

	void bs::File::readIncludes() {
		syntax->add(syntaxPackage());
		syntax->add(pkg);

		Auto<Parser> parser = CREATE(Parser, this, syntax, fileContents, file);
		headerSize = parser->parse(L"Includes");
		if (headerSize == Parser::NO_MATCH)
			throw parser->error();

		Auto<Object> includes = parser->transform();

		if (ArrayP<TypeName> *inc = as<ArrayP<TypeName>>(includes.borrow())) {
			setIncludes(inc);
		}
	}

	void bs::File::setIncludes(Par<ArrayP<TypeName>> inc) {
		for (nat i = 0; i < inc->count(); i++) {
			Auto<SimpleName> name = inc->at(i)->toName(scope);
			Auto<Named> found = pkg->engine().scope()->find(name);
			Package *p = as<Package>(found.borrow());
			if (!p)
				throw SyntaxError(SrcPos(file, 0), L"Unknown package " + ::toS(inc->at(i)));

			addInclude(scope, p);
		}

		syntax = getSyntax(scope);
	}

}
