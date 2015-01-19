#include "stdafx.h"
#include "PkgReader.h"
#include "Lib/Str.h"
#include "Engine.h"
#include "Scope.h"
#include "Exception.h"

namespace storm {

	Name readerName(const Name &name) {
		return name + Name(L"Reader");
	}

	Name syntaxPkg(const Path &path) {
		Name l(L"lang");
		return l + Name(path.ext());
	}

	hash_map<Name, PkgFiles *> syntaxPkg(const vector<Path> &paths, Engine &e) {
		typedef hash_map<Name, PkgFiles *> M;
		M r;

		for (nat i = 0; i < paths.size(); i++) {
			if (paths[i].isDir())
				continue;

			Name pkg = syntaxPkg(paths[i]);
			M::iterator found = r.find(pkg);
			PkgFiles *into;

			if (found == r.end()) {
				into = CREATE(PkgFiles, e);
				r.insert(make_pair(pkg, into));
			} else {
				into = found->second;
			}

			into->add(paths[i]);
		}

		return r;
	}

	/**
	 * PkgFiles.
	 */

	PkgFiles::PkgFiles() {}

	void PkgFiles::add(const Path &f) {
		files.push_back(f);
	}

	void PkgFiles::output(wostream &to) const {
		join(to, files, L", ");
	}

	/**
	 * PkgReader.
	 */

	PkgReader::PkgReader(Par<PkgFiles> files, Par<Package> owner) : pkgFiles(files), owner(owner) {}

	PkgReader::~PkgReader() {}

	void PkgReader::readSyntax(SyntaxRules &to) {}

	void PkgReader::readTypes() {}

	void PkgReader::readFunctions() {}


	/**
	 * FileReader.
	 */

	FileReader::FileReader(const Path &file, Par<Package> into) : file(file), package(into) {}

	void FileReader::readSyntax(SyntaxRules &to) {}

	void FileReader::readTypes() {}

	void FileReader::readFunctions() {}

	Package *FileReader::syntaxPackage() const {
		return package->engine.package(syntaxPkg(file));
	}

	/**
	 * FilesReader.
	 */

	FilesReader::FilesReader(Par<PkgFiles> files, Par<Package> pkg) : PkgReader(files, pkg) {}

	void FilesReader::readSyntax(SyntaxRules &to) {
		loadFiles();
		for (nat i = 0; i < files.size(); i++) {
			files[i]->readSyntax(to);
		}
	}

	void FilesReader::readTypes() {
		loadFiles();
		for (nat i = 0; i < files.size(); i++) {
			files[i]->readTypes();
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

		for (nat i = 0; i < pkgFiles->files.size(); i++) {
			files.push_back(createFile(pkgFiles->files[i]));
		}
	}

	FileReader *FilesReader::createFile(const Path &path) {
		throw InternalError(L"Please implement the 'createFile' on " + myType->name);
	}

}
