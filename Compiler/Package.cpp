#include "stdafx.h"
#include "Package.h"
#include "Core/StrBuf.h"
#include "Core/Str.h"
#include "Core/Set.h"
#include "Core/Io/Text.h"
#include "Gc/ObjMap.h"
#include "Engine.h"
#include "Reader.h"
#include "Exception.h"

namespace storm {

	Package::Package(Str *name) : NameSet(name), pkgPath(null), loading(null), discardOnLoad(true) {}

	Package::Package(Url *path) : NameSet(path->name()), pkgPath(path), loading(null), discardOnLoad(true) {
		engine().pkgMap()->put(pkgPath, this);

		documentation = new (this) PackageDoc(this);
	}

	Package::Package(Str *name, Url *path) : NameSet(name), pkgPath(path), discardOnLoad(true) {
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

	Bool Package::has(Named *item) {
		if (loading && loading->has(item))
			return true;

		return NameSet::has(item);
	}

	void Package::add(Named *item) {
		if (loading) {
			// If we should not allow duplicates, check for duplicates with ourself first. If this
			// would be a duplicate, try to add it to ourself to generate a suitable error message
			// (we know that 'add' will throw in this case).
			if (!loadingAllowDuplicates && NameSet::has(item))
				NameSet::add(item);

			loading->add(item);
			item->parentLookup = this;
		} else {
			NameSet::add(item);
		}
	}

	void Package::add(Template *item) {
		if (loading) {
			// We cannot really check for duplicates for templates.
			loading->add(item);
		} else {
			NameSet::add(item);
		}
	}

	Bool Package::remove(Named *item) {
		if (loading) {
			if (loading->remove(item))
				return true;
		}

		return NameSet::remove(item);
	}

	Bool Package::remove(Template *item) {
		if (loading) {
			if (loading->remove(item))
				return true;
		}

		return NameSet::remove(item);
	}

	MAYBE(Named *) Package::find(SimplePart *part, Scope source) {
		if (loading)
			if (Named *found = loading->find(part, source))
				return found;

		return NameSet::find(part, source);
	}

	NameSet::Iter Package::begin() const {
		if (loading)
			return NameSet::begin(loading);
		else
			return NameSet::begin();
	}

	NameSet::Iter Package::end() const {
		if (loading)
			return loading->end();
		else
			return NameSet::end();
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

	void Package::noDiscard() {
		discardOnLoad = false;
	}

	void Package::discardSource() {
		// We don't need to propagate this message, we emit it ourselves.
	}

	void Package::toS(StrBuf *to) const {
		if (parent())
			*to << L"Package " << identifier();
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
		// Load everything into a separate NameSet so that we can easily roll back in case of problems.
		loading = new (this) NameSet(name, params);
		loadingAllowDuplicates = false;

		try {
			Map<SimpleName *, PkgFiles *> *readers = readerName(files);
			Array<PkgReader *> *load = createReaders(readers);

			// Load everything!
			read(load);

			// All is well, merge with ourselves...
			NameSet::merge(loading);
			loading = null;

			// Ask functions to discard their sources.
			if (discardOnLoad)
				NameSet::discardSource();

		} catch (...) {
			// Discard the partially loaded results.
			loading = null;
			throw;
		}
	}

	static Str *noReaderWarning(SimpleName *name, Array<Url *> *files) {
		StrBuf *msg = new (name) StrBuf();
		*msg << L"No reader for [";
		Url *rootUrl = name->engine().package()->url();
		for (nat i = 0; i < files->count(); i++) {
			if (i > 0)
				*msg << L", ";
			*msg << files->at(i)->relative(rootUrl);
		}
		*msg << L"] (should be " << name << L")";

		return msg->toS();
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
			if (reader) {
				r->push(reader);
			} else {
				WARNING(noReaderWarning(i.k(), i.v()->files)->c_str());
			}
		}

		if (delayName && delayFiles) {
			PkgReader *reader = createReader(delayName, delayFiles->files, this);
			if (reader)
				r->push(reader);
		}

		return r;
	}

	void Package::reload() {
		if (!pkgPath)
			return;

		Array<Url *> *files = new (this) Array<Url *>();
		Array<Url *> *all = pkgPath->children();
		for (Nat i = 0; i < all->count(); i++)
			if (!all->at(i)->dir())
				files->push(all->at(i));

		reload(files, true);
	}

	void Package::reload(Url *file) {
		reload(new (this) Array<Url *>(file));
	}

	void Package::reload(Array<Url *> *files) {
		reload(files, false);
	}

	// Context for diffing entities during a reload.
	class ReloadContext : public NameDiff {
	public:
		ReloadContext(Array<Url *> *files, Bool complete) : update(files->engine().gc) {
			removeItems = new (files) Array<Named *>();
			removeTemplates = new (files) Array<Template *>();
			this->files = new (files) Set<Url *>();

			for (Nat i = 0; i < files->count(); i++)
				this->files->put(files->at(i));
		}

		// We don't need to worry about new things.
		virtual void added(Named *item) { PLN(L"New item: " << item); }
		virtual void added(Template *) {}

		// Old things, we just need to keep track of, so that we can remove them later.
		virtual void removed(Named *item) {
			if (handled(item->pos))
				removeItems->push(item);
		}
		virtual void removed(Template *item) {
			TODO(L"Handle templates(generators)! (they don't have a position)");
			removeTemplates->push(item);
		}

		virtual void changed(Named *old, Named *changed) {
			if (!handled(old->pos)) {
				// We have a duplicate definition!
				throw new (old) TypedefError(
					changed->pos,
					TO_S(old, changed << S(" is already defined at:\n@") << old->pos << S(": here")));
			}

			// Sanity-check the replacement now.
			changed->checkReplace(old);

			// We need to remove the old definition.
			removeItems->push(old);
			update.put(old, changed);
		}

		// Things we need to remove from the package before merging.
		Array<Named *> *removeItems;
		Array<Template *> *removeTemplates;

		// Things to update. "old" -> "new".
		ObjMap<Named> update;

		// Files we're examining. If null, we examine all files.
		Set<Url *> *files;

		// Is a particular entity handled by us? I.e., shall we treat it as if it exists?
		Bool handled(SrcPos pos) {
			if (!pos.file)
				return false;

			return !files || files->has(pos.file);
		}
	};

	void Package::reload(Array<Url *> *files, Bool complete) {
		loading = new (this) NameSet(name, params);
		loadingAllowDuplicates = true;

		ReloadContext context(files, complete);

		try {
			// This part is very similar to 'loadFiles'.
			Map<SimpleName *, PkgFiles *> *readers = readerName(files);
			Array<PkgReader *> *load = createReaders(readers);

			read(load);

			// Now, try to merge all entities inside 'loading', populating the 'context' variable
			// with what to do after performing some sanity checks of the update operations.
			NameSet::diff(loading, context);
		} catch (...) {
			// Discard the partially loaded results.
			loading = null;
			throw;
		}

		try {
			// Once this process is started, we can't roll it back anymore, so keep going as far as
			// possible, regardless of whether certain steps fail or not.
			for (Nat i = 0; i < context.update.count(); i++)
				context.update[i].to->replace(context.update[i].from);

			// Remove items to make the merge go smootly.
			for (Nat i = 0; i < context.removeItems->count(); i++)
				NameSet::remove(context.removeItems->at(i));
			for (Nat i = 0; i < context.removeTemplates->count(); i++)
				NameSet::remove(context.removeTemplates->at(i));

			// Now, we're done. We have resolved all conflicts, so now it is safe to merge the two
			// NameSets.
			NameSet::merge(loading);
			loading = null;

			TODO(L"Here we need to stop all threads, and update our heap.");
			// In particular, we need to:
			// - replace all occurrences of the old entities with the new ones (at least for types).
			// - modify the layout of any types requiring that.
		} catch (...) {
			// This indicates an internal error while merging. We will most likely not be able to
			// recover from this gracefully.
			TODO(L"How can we recover here?");
			loading = null;
			throw;
		}
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
