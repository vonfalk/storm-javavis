#include "stdafx.h"
#include "Package.h"

#include "BnfReader.h"
#include "Type.h"
#include "Function.h"
#include "PkgReader.h"
#include "Engine.h"
#include "Exception.h"

#include "Code/Function.h"

namespace storm {

	Package::Package(const String &name, Engine &engine) : NameSet(name), engine(engine), pkgPath(null) {
		init();
	}

	Package::Package(const Path &path, Engine &engine) : NameSet(path.title()), engine(engine) {
		pkgPath = new Path(path);
		pkgPath->makeDir();
		init();
	}

	NameLookup *Package::parent() const {
		return parentLookup;
	}

	void Package::init() {
		loaded = false;
		loading = false;
	}

	Package::~Package() {
		delete pkgPath;
	}

	void Package::output(std::wostream &to) const {
		if (parent() == null)
			to << "Root package";
		else
			to << "Pkg " << name;

		if (pkgPath)
			to << " in " << *pkgPath;
		else
			to << " (virtual)";
		to << endl;

		{
			Indent i(to);
			NameSet::output(to);
		}

		to << "Syntax:" << endl;
		{
			Indent i(to);
			to << syntaxRules;
		}
	}

	Named *Package::find(Par<NamePart> name) {
		if (Named *named = NameSet::find(name))
			return named;

		if (name->params.size() == 0)
			if (Package *pkg = loadPackage(name->name))
				return loadPackage(name->name);

		if (loaded)
			return null;

		load();
		return NameSet::find(name);
	}

	Package *Package::loadPackage(const String &name) {
		assert(NameSet::find(name, vector<Value>()) == null);

		// Virtual package, can not be auto loaded.
		if (pkgPath == null)
			return null;

		Path p = *pkgPath + Path(name);
		if (!p.exists())
			return null;

		Package *pkg = CREATE(Package, engine, *pkgPath + Path(name), engine);
		add(pkg);
		return pkg;
	}

	const SyntaxRules &Package::syntax() {
		load();
		return syntaxRules;
	}

	void Package::load() {
		if (!engine.initialized())
			return;
		if (loaded)
			return;
		if (!pkgPath)
			return;
		if (loading)
			return;
		loading = true;
		try {
			loadAlways();
		} catch (...) {
			loading = false;
			throw;
		}
		loading = false;
		loaded = true;
	}

	void Package::loadAlways() {
		typedef hash_map<Auto<Name>, PkgFiles *> M;
		M files;
		vector<PkgReader *> toLoad;

		try {
			files = syntaxPkg(pkgPath->children(), engine);

			Auto<Name> myName = path();

			for (M::iterator i = files.begin(); i != files.end(); ++i) {
				if (*(i->first) != *myName)
					addReader(toLoad, i->first, i->second);
			}

			if (files.count(myName) == 1)
				addReader(toLoad, myName, files[myName]);

			// Load all syntax.
			for (nat i = 0; i < toLoad.size(); i++) {
				toLoad[i]->readSyntax(syntaxRules);
			}

			// Load all types.
			for (nat i = 0; i < toLoad.size(); i++) {
				toLoad[i]->readTypes();
			}

			// Set up all types.
			for (nat i = 0; i < toLoad.size(); i++) {
				toLoad[i]->resolveTypes();
			}

			// Load all functions.
			for (nat i = 0; i < toLoad.size(); i++) {
				toLoad[i]->readFunctions();
			}

		} catch (...) {
			// We did nothing...
			syntaxRules.clear();
			NameSet::clear();
			throw;
		}

		releaseMap(files);
		releaseVec(toLoad);
	}

	PkgReader *Package::createReader(Par<Name> pkg, PkgFiles *files) {
		Auto<Name> rName = readerName(pkg);
		Type *readerT = as<Type>(engine.scope()->find(rName));
		if (!readerT) {
			// Ignore files that are not known.
			WARNING(L"Ignoring unknown filetype due to missing " << rName << L"(" << *files << L")");
			return null;
		}
		if (!readerT->isA(PkgReader::type(engine)))
			throw RuntimeError(::toS(rName) + L" is not a subtype of lang.PkgReader.");

		vector<Value> paramTypes(3);
		paramTypes[0] = Value();
		paramTypes[1] = Value(PkgFiles::type(engine));
		paramTypes[2] = Value(Package::type(engine));

		rName->add(Type::CTOR, paramTypes);
		Function *ctor = as<Function>(engine.scope()->find(rName));
		if (!ctor)
			throw RuntimeError(::toS(rName) + L": no constructor taking PkgFiles found!");

		code::FnCall call;
		call.param(files);
		call.param(this);
		PkgReader *r = create<PkgReader>(ctor, call);
		return r;
	}

	void Package::addReader(vector<PkgReader *> &to, Par<Name> pkg, PkgFiles *files) {
		PkgReader *r = createReader(pkg, files);
		if (r)
			to.push_back(r);
	}

}
