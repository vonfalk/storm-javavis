#include "stdafx.h"
#include "PkgReader.h"
#include "Lib/Str.h"

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

	Str *PkgFiles::toS() {
		return CREATE(Str, this, join(files, L", "));
	}

	/**
	 * PkgReader.
	 */

	PkgReader::PkgReader(PkgFiles *files) : files(files) {
		files->addRef();
	}

	PkgReader::~PkgReader() {
		files->release();
	}

	void PkgReader::readSyntax(SyntaxRules &to, Scope &scope) {}
}
