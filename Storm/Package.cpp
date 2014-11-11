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

	Package::Package(const String &name, Engine &engine) : Named(name), engine(engine), pkgPath(null) {
		init();
	}

	Package::Package(const Path &path, Engine &engine) : Named(path.title()), engine(engine) {
		pkgPath = new Path(path);
		pkgPath->makeDir();
		init();
	}

	void Package::init() {
		parentPkg = null;
		loaded = false;
		loading = false;
	}

	Package::~Package() {
		clearMap(packages);
		clearMap(types);
		clearMap(members);
		delete pkgPath;
	}

	Name Package::path() const {
		if (parentPkg)
			return parentPkg->path() + name;
		else
			return Name();
	}

	void Package::output(std::wostream &to) const {
		if (parentPkg == null)
			to << "Root package";
		else
			to << "Pkg " << name;

		if (pkgPath)
			to << " in " << *pkgPath;
		else
			to << " (virtual)";
		to << endl;

		to << "Loaded packages:" << endl;
		{
			Indent i(to);
			for (PkgMap::const_iterator i = packages.begin(); i != packages.end(); ++i) {
				to << *i->second << endl;
			}
		}

		to << "Types:" << endl;
		{
			Indent i(to);
			for (TypeMap::const_iterator i = types.begin(); i != types.end(); ++i) {
				to << *i->second << endl;
			}
		}

		to << "Members:" << endl;
		{
			Indent i(to);
			for (MemberMap::const_iterator i = members.begin(); i != members.end(); ++i) {
				to << *i->second << endl;
			}
		}

		to << "Syntax:" << endl;
		{
			Indent i(to);
			to << syntaxRules;
		}
	}

	Named *Package::find(const Name &name) {
		return findName(name, 0);
	}

	Named *Package::findName(const Name &name, nat start) {
		if (start == name.size())
			return this;
		if (start > name.size())
			return null;

		Package *found = childPackage(name[start]);
		if (found == null)
			return findTypeOrFn(name, start);

		return found->findName(name, start + 1);
	}

	Named *Package::findTypeOrFn(const Name &name, nat start) {
		TypeMap::iterator i = types.find(name[start]);
		if (i != types.end()) {
			Type *t = i->second;
			if (name.size() - 1 == start)
				return t;
			else
				return t->find(name.from(start + 1));
		}

		// No nested types.
		if (start != name.size() - 1)
			return null;
		MemberMap::iterator j = members.find(name[start]);
		if (j != members.end())
			return j->second;

		return null;
	}

	Package *Package::loadPackage(const String &name) {
		assert(packages.count(name) == 0);

		// Virtual package, can not be auto loaded.
		if (pkgPath == null)
			return null;

		Path p = *pkgPath + Path(name);
		if (!p.exists())
			return null;

		Package *pkg = new Package(*pkgPath + Path(name), engine);
		packages[name] = pkg;
		pkg->parentPkg = this;
		return pkg;
	}

	const SyntaxRules &Package::syntax() {
		load();
		return syntaxRules;
	}

	Package *Package::childPackage(const String &name) {
		PkgMap::iterator i = packages.find(name);
		if (i == packages.end())
			return loadPackage(name);
		else
			return i->second;
	}

	void Package::add(Package *pkg, const String &name) {
		assert(packages.count(name) == 0);
		packages.insert(make_pair(name, pkg));
		pkg->parentPkg = this;
	}

	void Package::add(Type *type) {
		assert(types.count(type->name) == 0);
		types.insert(make_pair(type->name, type));
		type->parentPkg = this;
	}

	void Package::add(NameOverload *fn) {
		Overload *o = null;
		MemberMap::iterator i = members.find(fn->name);
		if (i == members.end()) {
			o = new Overload(fn->name);
			members.insert(make_pair(fn->name, o));
		} else {
			o = i->second;
		}

		o->add(fn);
	}

	void Package::load() {
		if (loaded)
			return;
		if (!pkgPath)
			return;
		if (loading)
			throw InternalError(L"Recursive package loading detected! " + toS());
		loading = true;
		loadAlways();
		loading = false;
		loaded = true;
	}

	void Package::loadAlways() {
		typedef hash_map<Name, PkgFiles *> M;
		M files;

		try {
			files = syntaxPkg(pkgPath->children(), engine);

			Name myName = path();
			for (M::iterator i = files.begin(); i != files.end(); ++i) {
				if (i->first != myName)
					load(i->first, i->second);
			}

			if (files.count(myName) == 1) {
				load(myName, files[myName]);
			}

		} catch (...) {
			// We did nothing...
			syntaxRules.clear();
			clearMap(types);
			clearMap(members);
			releaseMap(files);
			throw;
		}

		releaseMap(files);
	}

	PkgReader *Package::createReader(const Name &pkg, PkgFiles *files) {
		Name rName = readerName(pkg);
		Type *readerT = as<Type>(engine.scope()->find(rName));
		if (!readerT) {
			// Ignore files that are not known.
			WARNING(L"Ignoring unknown filetype due to missing " << rName);
			return null;
		}
		if (readerT->super() != PkgReader::type(engine))
			throw RuntimeError(::toS(rName) + L" is not a subtype of lang.PkgReader.");

		vector<Value> paramTypes(2);
		paramTypes[0] = Value(engine.typeType()); // TODO: Type::type(engine)
		paramTypes[1] = Value(PkgFiles::type(engine));
		Function *ctor = as<Function>(engine.scope()->find(rName + Name(Type::CTOR), paramTypes));
		if (!ctor)
			throw RuntimeError(::toS(rName) + L": no constructor taking PkgFiles found!");

		code::FnCall call;
		call.param(readerT);
		call.param(files);
		return call.call<PkgReader *>(ctor->pointer());
	}

	void Package::load(const Name &pkg, PkgFiles *files) {
		PkgReader *reader = createReader(pkg, files);
		if (!reader)
			return;

		try {
			Scope scope(this);
			reader->readSyntax(syntaxRules, scope);

		} catch (...) {
			reader->release();
		}
		reader->release();
	}

}
