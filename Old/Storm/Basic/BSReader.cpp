#include "stdafx.h"
#include "BSReader.h"
#include "BSTemplate.h"
#include "BSContents.h"
#include "BSClass.h"
#include "Syntax/Parser.h"
#include "Engine.h"
#include "SyntaxEnv.h"
#include "Shared/Io/Text.h"


namespace storm {

	using syntax::Parser;

	bs::Reader::Reader(Par<PkgFiles> files, Par<Package> pkg) : FilesReader(files, pkg) {}

	FileReader *bs::Reader::createFile(Par<Url> path) {
		return CREATE(bs::File, this, path, pkg);
	}


	bs::File::File(Par<Url> path, Par<Package> owner)
		: FileReader(path, owner), scopeLookup(CREATE(BSScope, engine())), scope(owner, scopeLookup) {

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

		Auto<Parser> parser = Parser::create(syntaxPackage(), L"SFile");
		addSyntax(scope, parser);

		parser->parse(fileContents, file, headerEnd);
		if (parser->hasError())
			throw parser->error();

		Auto<SyntaxEnv> env = CREATE(SyntaxEnv, this, scope);
		contents = parser->transform<Contents, SyntaxEnv>(env);
	}

	void bs::File::readIncludes() {
		Auto<Parser> parser = Parser::create(syntaxPackage(), L"SIncludes");
		parser->addSyntax(pkg);

		headerEnd = parser->parse(fileContents, file);
		if (!parser->hasTree())
			parser->throwError();

		setIncludes(steal(parser->transform<ArrayP<SrcName>>()));
	}

	void bs::File::setIncludes(Par<ArrayP<SrcName>> inc) {
		for (nat i = 0; i < inc->count(); i++) {
			Auto<SrcName> name = inc->at(i);
			Auto<Named> found = pkg->engine().scope()->find(name);
			Package *p = as<Package>(found.borrow());
			if (!p)
				throw SyntaxError(SrcPos(file, 0), L"Unknown package " + ::toS(name));

			addInclude(scope, p);
		}
	}

}
