#include "stdafx.h"
#include "PkgReader.h"
#include "Shared/Str.h"
#include "Engine.h"
#include "Scope.h"
#include "Exception.h"
#include "Type.h"

namespace storm {

	Name *readerName(Par<const Name> name) {
		Name *n = CREATE(Name, name, name);
		n->add(steal(CREATE(NamePart, name, L"Reader")));
		return n;
	}

	Name *syntaxPkg(Par<Url> path) {
		Name *n = CREATE(Name, path, L"lang");
		Auto<NamePart> part = CREATE(NamePart, path, steal(path->ext()));
		n->add(part);
		return n;
	}

	hash_map<Auto<Name>, Auto<PkgFiles> > syntaxPkg(Auto<ArrayP<Url>> paths) {
		typedef hash_map<Auto<Name>, Auto<PkgFiles> > M;
		M r;

		for (nat i = 0; i < paths->count(); i++) {
			if (paths->at(i)->dir())
				continue;

			Auto<Name> pkg = syntaxPkg(paths->at(i));
			M::iterator found = r.find(pkg);

			Auto<PkgFiles> into;
			if (found == r.end()) {
				into = CREATE(PkgFiles, paths);
				r.insert(make_pair(pkg, into));
			} else {
				into = found->second;
			}

			into->add(paths->at(i));
		}

		return r;
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

	void PkgReader::readSyntax() {}

	void PkgReader::readTypes() {}

	void PkgReader::resolveTypes() {}

	void PkgReader::readFunctions() {}


	/**
	 * FileReader.
	 */

	FileReader::FileReader(Par<Url> file, Par<Package> into) : file(file), pkg(into) {}

	void FileReader::readSyntax() {}

	void FileReader::readTypes() {}

	void FileReader::resolveTypes() {}

	void FileReader::readFunctions() {}

	Package *FileReader::syntaxPackage() const {
		Auto<Name> pkg = syntaxPkg(file);
		return engine().package(pkg);
	}

	/**
	 * FilesReader.
	 */

	FilesReader::FilesReader(Par<PkgFiles> files, Par<Package> pkg) : PkgReader(files, pkg) {}

	void FilesReader::readSyntax() {
		loadFiles();
		for (nat i = 0; i < files.size(); i++) {
			files[i]->readSyntax();
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
