#include "stdafx.h"
#include "Reader.h"
#include "Core/Str.h"
#include "Exception.h"
#include "Function.h"
#include "FileReader.h"
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
		return readerName(file->ext());
	}

	SimpleName *readerName(Str *ext) {
		Engine &e = ext->engine();
		SimpleName *r = new (e) SimpleName();
		r->add(new (e) Str(L"lang"));
		r->add(ext);
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

		// TODO: Make sure to use the proper thread when calling 'createFn'!
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

	FileReader *PkgReader::readFile(Url *url, Str *src) {
		return null;
	}


	FilePkgReader::FilePkgReader(Array<Url *> *files, Package *pkg, Fn<FileReader *, FileInfo *> *create)
		: PkgReader(files, pkg), create(create) {}

	void FilePkgReader::loadReaders() {
		if (readers)
			return;

		readers = new (this) Array<FileReader *>();
		readers->reserve(files->count());
		for (nat i = 0; i < files->count(); i++) {
			FileReader *r = create->call(new (this) FileInfo(files->at(i), pkg));
			if (!r)
				throw InternalError(L"Can not use a null FileReader in a FilePkgReader!");
			readers->push(r);
		}
	}

	typedef void (CODECALL FileReader::*FileMember)();
	static void traverse(FileReader *reader, FileMember member, ReaderQuery q) {
		while (reader) {
			(reader->*member)();
			reader = reader->next(q);
		}
	}

	void FilePkgReader::readSyntaxRules() {
		loadReaders();
		for (nat i = 0; i < readers->count(); i++)
			traverse(readers->at(i), &FileReader::readSyntaxRules, qSyntax);
	}

	void FilePkgReader::readSyntaxProductions() {
		loadReaders();
		for (nat i = 0; i < readers->count(); i++)
			traverse(readers->at(i), &FileReader::readSyntaxProductions, qSyntax);
	}

	void FilePkgReader::readTypes() {
		loadReaders();
		for (nat i = 0; i < readers->count(); i++)
			traverse(readers->at(i), &FileReader::readTypes, qTypes);
	}

	void FilePkgReader::resolveTypes() {
		loadReaders();
		for (nat i = 0; i < readers->count(); i++)
			traverse(readers->at(i), &FileReader::resolveTypes, qTypes);
	}

	void FilePkgReader::readFunctions() {
		loadReaders();
		for (nat i = 0; i < readers->count(); i++)
			traverse(readers->at(i), &FileReader::readFunctions, qFunctions);
	}

	FileReader *FilePkgReader::readFile(Url *file, Str *src) {
		return create->call(new (this) FileInfo(src, src->begin(), file, pkg));
	}

}
