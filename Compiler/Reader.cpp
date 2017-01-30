#include "stdafx.h"
#include "Reader.h"
#include "Core/Str.h"
#include "Exception.h"
#include "Function.h"
#include "Engine.h"

namespace storm {

	PkgFiles::PkgFiles() {
		files = new (this) Array<Url *>();
	}

	void PkgFiles::deepCopy(CloneEnv *e) {
		cloned(files, e);
	}


	SimpleName *syntaxPkgName(Url *file) {
		SimpleName *r = new (file) SimpleName();
		r->add(new (file) Str(L"lang"));
		r->add(file->ext());
		return r;
	}

	SimpleName *readerName(Url *file) {
		Engine &e = file->engine();
		SimpleName *r = new (e) SimpleName();
		r->add(new (e) Str(L"lang"));
		r->add(file->ext());
		Array<Value> *params = new (e) Array<Value>();
		params->push(thisPtr(Array<Url *>::stormType(e)));
		params->push(thisPtr(Package::stormType(e)));
		r->add(new (e) SimplePart(new (e) Str(L"reader"), params));
		return r;
	}

	Map<SimpleName *, PkgFiles *> *readerName(Array<Url *> *files) {
		Map<SimpleName *, PkgFiles *> *r = new (files) Map<SimpleName *, PkgFiles *>();
		for (nat i = 0; i < files->count(); i++) {
			Url *f = files->at(i);
			SimpleName *reader = readerName(f);
			r->at(reader)->files->push(f);
		}
		return r;
	}

	MAYBE(PkgReader *) createReader(Array<Url *> *files, Package *pkg) {
		if (files->empty())
			return null;
		Url *f = files->at(0);
		return createReader(readerName(f), files, pkg);
	}

	MAYBE(PkgReader *) createReader(SimpleName *name, Array<Url *> *files, Package *pkg) {
		Function *createFn = as<Function>(pkg->engine().scope().find(name));
		if (!createFn) {
			StrBuf *msg = new (name) StrBuf();
			*msg << L"No reader for [";
			Url *rootUrl = pkg->engine().package()->url();
			for (nat i = 0; i < files->count(); i++) {
				if (i > 0)
					*msg << L", ";
				*msg << files->at(i)->relative(rootUrl);
			}
			*msg << L"] (should be " << name << L")";
			WARNING(msg->toS()->c_str());
			return null;
		}

		Value pkgReader = thisPtr(PkgReader::stormType(pkg->engine()));
		if (!pkgReader.canStore(createFn->result)) {
			StrBuf *msg = new (name) StrBuf();
			*msg << L"Invalid return type for " << createFn << L": expected " << pkgReader;
			throw LangDefError(msg->toS()->c_str());
		}

		typedef PkgReader *(*Fn)(Array<Url *> *, Package *);
		Fn fn = (Fn)createFn->ref().address();
		return (*fn)(files, pkg);
	}


	PkgReader::PkgReader(Array<Url *> *files, Package *pkg) : pkg(pkg), files(files) {}

	void PkgReader::readSyntaxRules() {}

	void PkgReader::readSyntaxProductions() {}

	void PkgReader::readTypes() {}

	void PkgReader::resolveTypes() {}

	void PkgReader::readFunctions() {}

	FileReader *PkgReader::readFile(Url *url) {
		return null;
	}


	FileReader::FileReader(Url *file, Package *pkg) : pkg(pkg), file(file) {}

	void FileReader::readSyntaxRules() {}

	void FileReader::readSyntaxProductions() {}

	void FileReader::readTypes() {}

	void FileReader::resolveTypes() {}

	void FileReader::readFunctions() {}

	syntax::Rule *FileReader::rootRule() {
		throw LangDefError(L"This language does not support syntax highlighting.");
	}

	syntax::InfoParser *FileReader::createParser() {
		return new (this) syntax::InfoParser(rootRule());
	}


	FilePkgReader::FilePkgReader(Array<Url *> *files, Package *pkg, Fn<FileReader *, Url *, Package *> *create)
		: PkgReader(files, pkg), create(create) {}

	void FilePkgReader::loadReaders() {
		if (readers)
			return;

		readers = new (this) Array<FileReader *>();
		readers->reserve(files->count());
		for (nat i = 0; i < files->count(); i++) {
			FileReader *r = create->call(files->at(i), pkg);
			if (!r)
				throw InternalError(L"Can not use a null FileReader in a FilePkgReader!");
			readers->push(r);
		}
	}

	void FilePkgReader::readSyntaxRules() {
		loadReaders();
		for (nat i = 0; i < readers->count(); i++)
			readers->at(i)->readSyntaxRules();
	}

	void FilePkgReader::readSyntaxProductions() {
		loadReaders();
		for (nat i = 0; i < readers->count(); i++)
			readers->at(i)->readSyntaxProductions();
	}

	void FilePkgReader::readTypes() {
		loadReaders();
		for (nat i = 0; i < readers->count(); i++)
			readers->at(i)->readTypes();
	}

	void FilePkgReader::resolveTypes() {
		loadReaders();
		for (nat i = 0; i < readers->count(); i++)
			readers->at(i)->resolveTypes();
	}

	void FilePkgReader::readFunctions() {
		loadReaders();
		for (nat i = 0; i < readers->count(); i++)
			readers->at(i)->readFunctions();
	}

	FileReader *FilePkgReader::readFile(Url *file) {
		loadReaders();
		for (nat i = 0; i < readers->count(); i++)
			if (readers->at(i)->file->equals(file))
				return readers->at(i);
		return null;
	}

}
