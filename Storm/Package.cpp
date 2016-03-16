#include "stdafx.h"
#include "Package.h"

#include "Type.h"
#include "Function.h"
#include "PkgReader.h"
#include "Engine.h"
#include "Exception.h"

#include "Code/FnParams.h"

namespace storm {

	Package::Package(const String &name) : NameSet(name), pkgPath(null) {}

	Package::Package(Par<Url> path) : NameSet(steal(path->title())->v), pkgPath(path) {}

	NameLookup *Package::parent() const {
		return parentLookup;
	}

	Url *Package::url() const {
		return pkgPath.ret();
	}

	Package::~Package() {}

	void Package::output(std::wostream &to) const {
		if (parent() == null)
			to << "Root package";
		else
			to << "Pkg " << identifier();

		if (pkgPath)
			to << " in " << *pkgPath;
		else
			to << " (virtual)";
		to << endl;

		{
			Indent i(to);
			NameSet::output(to);
		}
	}

	Named *Package::loadName(Par<SimplePart> part) {
		// We are only loading packages this way at the moment.
		if (part->empty()) {
			if (Package *pkg = loadPackage(part->name)) {
				return pkg;
			}
		}

		return null;
	}

	bool Package::loadAll() {
		// Do not load things during compiler startup. Doing that will most probably fail
		// since we are in the process of setting things up.
		if (!engine().initialized())
			return false;

		if (!pkgPath)
			return true;

		Auto<ArrayP<Url>> children = pkgPath->children();

		// Load any remaining packages...
		for (nat i = 0; i < children->count(); i++) {
			Auto<Url> &now = children->at(i);
			if (now->dir()) {
				Auto<Str> name = now->name();
				Auto<SimplePart> part = CREATE(SimplePart, this, name);
				Auto<Named> loaded = tryFind(part);
				if (!loaded) {
					add(steal(loadPackage(name->v)));
				}
			}
		}

		// Load all code.
		loadFiles(children);

		return true;
	}

	Package *Package::loadPackage(const String &name) {
		// Virtual package, can not be auto loaded.
		if (pkgPath == null)
			return null;

		// TODO: Make sure this is case sensitive!
		Auto<Url> sub = pkgPath->push(name);
		if (!sub->exists())
			return null;

		return CREATE(Package, this, sub);
	}

	void Package::loadFiles(Auto<ArrayP<Url>> children) {
		typedef MAP_PP(SimpleName, PkgFiles) M;

		// Remember previous contents if things go wrong...
		vector<Auto<Named>> prev;
		for (Iter i = begin(), to = end(); i != to; ++i)
			prev.push_back(*i);

		try {
			// These are here, so that they are freed properly if things go south.
			vector<Auto<PkgReader> > toLoad;
			Auto<M> files = syntaxPkg(children);

			Auto<SimpleName> myName = path();

			for (M::Iter i = files->begin(); i != files->end(); ++i) {
				if (*i.key() != *myName)
					addReader(toLoad, i.key(), i.val());
			}

			if (files->has(myName))
				addReader(toLoad, myName, files->get(myName));

			// Load all syntax.
			for (nat i = 0; i < toLoad.size(); i++) {
				toLoad[i]->readSyntaxRules();
			}

			for (nat i = 0; i < toLoad.size(); i++) {
				toLoad[i]->readSyntaxOptions();
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
			// Figure out which to remove and which to keep...
			for (Iter i = begin(), to = end(); i != to; ++i) {
				if (Type *t = as<Type>(i->borrow())) {
					for (nat i = 0; i < prev.size(); i++) {
						if (prev[i].borrow() == t) {
							t->clear();
							break;
						}
					}
				}
			}

			// Remove all types and re-add the ones in 'prev' to restore the state.
			NameSet::clear();

			for (nat i = 0; i < prev.size(); i++) {
				NameSet::add(prev[i]);
			}

			prev.clear();

			TODO(L"Handle templates as well! Not entirely bullet-proof solution either...");

			//NameSet::clear();
			//TODO(L"We can not clear everything here, that removes potential built-in types in the same pkg!");
			throw;
		}
	}

	PkgReader *Package::createReader(Par<SimpleName> pkg, Par<PkgFiles> files) {
		Auto<SimpleName> rName = readerName(pkg);
		Auto<Type> readerT = steal(engine().scope()->find(rName)).as<Type>();
		if (!readerT) {
			// Ignore files that are not known.
			WARNING(L"Ignoring unknown filetype due to missing " << rName << L"(" << *files << L")");
			return null;
		}
		if (!readerT->isA(PkgReader::stormType(this)))
			throw RuntimeError(::toS(rName) + L" is not a subtype of lang.PkgReader.");

		vector<Value> paramTypes(3);
		paramTypes[0] = Value::thisPtr(readerT);
		paramTypes[1] = Value(PkgFiles::stormType(this));
		paramTypes[2] = Value(Package::stormType(this));

		Auto<Function> ctor = steal(readerT->findCpp(Type::CTOR, paramTypes)).as<Function>();
		if (!ctor)
			throw RuntimeError(::toS(rName) + L": no constructor taking PkgFiles found!");

		os::FnParams params;
		params.add(files.borrow());
		params.add(this);
		PkgReader *r = create<PkgReader>(ctor, params);
		return r;
	}

	void Package::addReader(vector<Auto<PkgReader> > &to, Par<SimpleName> pkg, Par<PkgFiles> files) {
		PkgReader *r = createReader(pkg, files);
		if (r)
			to.push_back(r);
	}

	/**
	 * Helpers.
	 */

	Package *rootPkg(EnginePtr e) {
		Package *root = e.v.rootPackage();
		root->addRef();
		return root;
	}

}
