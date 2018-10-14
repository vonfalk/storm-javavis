#include "stdafx.h"
#include "Package.h"
#include "Core/StrBuf.h"
#include "Core/Str.h"
#include "Core/Io/Text.h"
#include "Engine.h"
#include "Reader.h"
#include "Exception.h"

namespace storm {

	Package::Package(Str *name) : NameSet(name) {}

	Package::Package(Url *path) : NameSet(path->name()), pkgPath(path) {
		engine().pkgMap()->put(pkgPath, this);

		documentation = new (this) PackageDoc(this);
	}

	Package::Package(Str *name, Url *path) : NameSet(name), pkgPath(path) {
		engine().pkgMap()->put(pkgPath, this);

		documentation = new (this) PackageDoc(this);
	}

	NameLookup *Package::parent() const {
		// We need to be able to return null.
		return parentLookup;
	}

	Url *Package::url() const {
		return pkgPath;
	}

	void Package::setUrl(Url *url) {
		assert(!engine().has(bootDone), L"Shall not be done after boot is complete!");
		if (pkgPath)
			engine().pkgMap()->remove(pkgPath);
		pkgPath = url;
		engine().pkgMap()->put(pkgPath, this);

		for (Iter i = begin(), e = end(); i != e; ++i) {
			if (Package *p = as<Package>(i.v())) {
				if (p->params->count() > 0)
					continue;
				Url *sub = url->pushDir(p->name);
				if (!sub->exists())
					continue;

				p->setUrl(sub);
			}
		}

		if (!documentation)
			documentation = new (this) PackageDoc(this);
	}

	Bool Package::loadName(SimplePart *part) {
		// We're only loading packages this way.
		if (part->params->empty()) {
			if (Package *pkg = loadPackage(part->name)) {
				add(pkg);
				return true;
			}
		}

		// There may be other things to load!
		return false;
	}

	Bool Package::loadAll() {
		// Do not load things before the compiler has properly started. As everything is a chaos at
		// that time, loading anything external will most likely fail. So we retry later.
		if (!engine().has(bootDone))
			return false;

		// Nothing to load.
		if (!pkgPath)
			return true;

		Array<Url *> *children = pkgPath->children();
		Array<Url *> *files = new (this) Array<Url *>();

		// Load any remaining packages:
		for (Nat i = 0; i < children->count(); i++) {
			Url *now = children->at(i);
			if (now->dir()) {
				Str *name = now->name();
				if (!tryFind(new (this) SimplePart(name), Scope()))
					add(loadPackage(name));
			} else {
				files->push(now);
			}
		}

		// Load all code.
		loadFiles(files);

		return true;
	}

	void Package::toS(StrBuf *to) const {
		if (parent())
			*to << L"Pkg " << identifier();
		else
			*to << L"Root package (" << name << L")";

		if (pkgPath)
			*to << L"(in " << pkgPath << L")";
		else
			*to << L"(virtual)";

		if (false) {
			*to << L"\n";
			Indent z(to);
			NameSet::toS(to);
		}
	}

	Package *Package::loadPackage(Str *name) {
		// Virtual package, can not be auto loaded.
		if (!pkgPath)
			return null;

		// Name parts that are empty do not play well with the Url.
		if (name->empty())
			return null;

		// TODO: Make sure this is case sensitive!
		Url *sub = pkgPath->push(name);
		if (!sub->exists())
			return null;

		return new (this) Package(sub);
	}

	void Package::loadFiles(Array<Url *> *files) {
		// TODO: Remember previous contents if things go wrong...
		// Array<Named *> *prev = new (this) Array<Named *>();
		// for (Iter i = begin(), to = end(); i != to; ++i)
		// 	prev->push(i.v());

		try {
			Map<SimpleName *, PkgFiles *> *readers = readerName(files);
			Array<PkgReader *> *load = createReaders(readers);

			for (Nat i = 0; i < load->count(); i++)
				load->at(i)->readSyntaxRules();

			for (Nat i = 0; i < load->count(); i++)
				load->at(i)->readSyntaxProductions();

			for (Nat i = 0; i < load->count(); i++)
				load->at(i)->readTypes();

			for (Nat i = 0; i < load->count(); i++)
				load->at(i)->resolveTypes();

			for (Nat i = 0; i < load->count(); i++)
				load->at(i)->readFunctions();

			for (Nat i = 0; i < load->count(); i++)
				load->at(i)->resolveFunctions();

		} catch (...) {
			TODO(L"Try to restore!");
			throw;
		}
	}

	Array<PkgReader *> *Package::createReaders(Map<SimpleName *, PkgFiles *> *readers) {
		typedef Map<SimpleName *, PkgFiles *> ReaderMap;
		Array<PkgReader *> *r = new (this) Array<PkgReader *>();
		SimpleName *me = path();

		SimpleName *delayName = null;
		PkgFiles *delayFiles = null;

		for (ReaderMap::Iter i = readers->begin(), end = readers->end(); i != end; ++i) {
			SimpleName *name = i.k();

			// Load ourselves last.
			if (*name->parent() == *me) {
				delayName = name;
				delayFiles = i.v();
				continue;
			}

			PkgReader *reader = createReader(i.k(), i.v()->files, this);
			if (reader)
				r->push(reader);
		}

		if (delayName && delayFiles) {
			PkgReader *reader = createReader(delayName, delayFiles->files, this);
			if (reader)
				r->push(reader);
		}

		return r;
	}

	/**
	 * Documentation
	 */

	PackageDoc::PackageDoc(Package *owner) : pkg(owner) {}

	Doc *PackageDoc::get() {
		Doc *result = doc(pkg);

		if (Url *url = pkg->url()) {
			Url *file = url->push(new (this) Str(S("README")));
			if (file->exists())
				result->body = readAllText(file);
		}

		return result;
	}

	/**
	 * Helpers for Storm.
	 */

	MAYBE(Package *) package(Url *path) {
		return path->engine().package(path);
	}

	Package *rootPkg(EnginePtr e) {
		return e.v.package();
	}

}
