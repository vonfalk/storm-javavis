#include "stdafx.h"
#include "PkgReader.h"
#include "Shared/Str.h"
#include "Engine.h"
#include "Scope.h"
#include "Exception.h"
#include "Type.h"

namespace storm {

	SimpleName *readerName(Par<SimpleName> name) {
		SimpleName *n = CREATE(SimpleName, name, name);
		n->add(steal(CREATE(SimplePart, name, L"Reader")));
		return n;
	}

	SimpleName *syntaxPkg(Par<Url> path) {
		SimpleName *n = CREATE(SimpleName, path);
		Auto<SimplePart> part = CREATE(SimplePart, path, L"lang");
		n->add(part);
		part = CREATE(SimplePart, path, steal(path->ext()));
		n->add(part);
		return n;
	}

	MAP_PP(SimpleName, PkgFiles) *syntaxPkg(Auto<ArrayP<Url>> paths) {
		Auto<MAP_PP(SimpleName, PkgFiles)> r = CREATE(MAP_PP(SimpleName, PkgFiles), paths);

		for (nat i = 0; i < paths->count(); i++) {
			if (paths->at(i)->dir())
				continue;

			Auto<SimpleName> pkg = syntaxPkg(paths->at(i));

			Auto<PkgFiles> into;
			if (r->has(pkg)) {
				into = r->get(pkg);
			} else {
				into = CREATE(PkgFiles, paths);
				r->put(pkg, into);
			}

			into->add(paths->at(i));
		}

		return r.ret();
	}

	/**
	 * PkgFiles.
	 */

	PkgFiles::PkgFiles() {
		files = CREATE(ArrayP<Url>, engine());
	}

	void PkgFiles::add(Par<Url> f) {
		files->push(f);
	}

	void PkgFiles::output(wostream &to) const {
		to << files;
	}

	/**
	 * PkgReader.
	 */

	PkgReader::PkgReader(Par<PkgFiles> files, Par<Package> owner) : pkgFiles(files), pkg(owner) {}

	PkgReader::~PkgReader() {}

	void PkgReader::readSyntaxRules() {}

	void PkgReader::readSyntaxOptions() {}

	void PkgReader::readTypes() {}

	void PkgReader::resolveTypes() {}

	void PkgReader::readFunctions() {}


	/**
	 * FileReader.
	 */

	FileReader::FileReader(Par<Url> file, Par<Package> into) : file(file), pkg(into) {}

	void FileReader::readSyntaxRules() {}

	void FileReader::readSyntaxOptions() {}

	void FileReader::readTypes() {}

	void FileReader::resolveTypes() {}

	void FileReader::readFunctions() {}

	Package *FileReader::syntaxPackage() const {
		Auto<SimpleName> pkg = syntaxPkg(file);
		return engine().package(pkg);
	}

	/**
	 * FilesReader.
	 */

	FilesReader::FilesReader(Par<PkgFiles> files, Par<Package> pkg) : PkgReader(files, pkg) {}

	void FilesReader::readSyntaxRules() {
		loadFiles();
		for (nat i = 0; i < files.size(); i++) {
			files[i]->readSyntaxRules();
		}
	}

	void FilesReader::readSyntaxOptions() {
		loadFiles();
		for (nat i = 0; i < files.size(); i++) {
			files[i]->readSyntaxOptions();
		}
	}

	void FilesReader::readTypes() {
		loadFiles();
		for (nat i = 0; i < files.size(); i++) {
			files[i]->readTypes();
		}
	}

	void FilesReader::resolveTypes() {
		loadFiles();
		for (nat i = 0; i < files.size(); i++) {
			files[i]->resolveTypes();
		}
	}

	void FilesReader::readFunctions() {
		loadFiles();
		for (nat i = 0; i < files.size(); i++) {
			files[i]->readFunctions();
		}
	}

	void FilesReader::loadFiles() {
		if (files.size() != 0)
			return;

		for (nat i = 0; i < pkgFiles->files->count(); i++) {
			files.push_back(createFile(pkgFiles->files->at(i)));
		}
	}

	FileReader *FilesReader::createFile(Par<Url> path) {
		throw InternalError(L"Please implement the 'createFile' on " + myType->name);
	}

}
