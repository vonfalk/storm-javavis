#include "stdafx.h"
#include "NameSet.h"
#include "Engine.h"
#include "Exception.h"
#include "Core/StrBuf.h"
#include "Core/Str.h"

namespace storm {

	NameOverloads::NameOverloads() :
		items(new (this) Array<Named *>()),
		templates(new (this) Array<Template *>()) {}

	Nat NameOverloads::count() const {
		return items->count();
	}

	Named *NameOverloads::operator [](Nat id) const {
		return items->at(id);
	}

	Named *NameOverloads::at(Nat id) const {
		return items->at(id);
	}

	static bool equals(Array<Value> *a, Array<Value> *b) {
		if (a->count() != b->count())
			return false;

		for (nat i = 0; i < a->count(); i++)
			if (a->at(i) != b->at(i))
				return false;

		return true;
	}

	void NameOverloads::add(Named *item) {
		for (nat i = 0; i < items->count(); i++)
			if (storm::equals(item->params, items->at(i)->params))
				throw TypedefError(::toS(item) + L" is already defined as " + ::toS(items->at(i)->identifier()));

		items->push(item);
	}

	void NameOverloads::add(Template *item) {
		// There is no way to validate templates at this stage.
		templates->push(item);
	}

	Named *NameOverloads::createTemplate(SimplePart *part) {
		Named *found = null;
		for (nat i = 0; i < templates->count(); i++) {
			Named *n = templates->at(i)->generate(part);
			if (found != null && n != null) {
				throw TypedefError(L"Multiple template matches for: " + ::toS(part));
			} else if (n) {
				found = n;
			}
		}

		if (found)
			add(found);
		return found;
	}

	void NameOverloads::toS(StrBuf *to) const {
		for (nat i = 0; i < items->count(); i++)
			*to << items->at(i) << L"\n";
		if (templates->any())
			*to << L"<" << templates->count() << L" templates>\n";
	}


	/**
	 * NameSet.
	 */

	NameSet::NameSet(Str *name) : Named(name) {
		init();
	}

	NameSet::NameSet(Str *name, Array<Value> *params) : Named(name, params) {
		init();
	}

	void NameSet::init() {
		loaded = false;
		loading = false;
		nextAnon = null;

		if (engine().has(bootTemplates))
			lateInit();
	}

	void NameSet::lateInit() {
		Named::lateInit();

		overloads = new (this) Map<Str *, NameOverloads *>();
	}

	void NameSet::add(Named *item) {
		overloads->at(item->name)->add(item);
		item->parentLookup = this;
	}

	void NameSet::add(Template *item) {
		overloads->at(item->name)->add(item);
	}

	Str *NameSet::anonName() {
		StrBuf *buf = new (this) StrBuf();
		*buf << L"@ " << (nextAnon++);
		return buf->toS();
	}

	Array<Named *> *NameSet::content() {
		Array<Named *> *r = new (this) Array<Named *>();
		for (Overloads::Iter at = overloads->begin(); at != overloads->end(); ++at) {
			NameOverloads &o = *at.v();
			for (nat i = 0; i < o.count(); i++)
				r->push(o[i]);
		}
		return r;
	}

	void NameSet::forceLoad() {
		if (loaded)
			return;

		if (loading) {
			// This happens quite a lot...
			// WARNING(L"Recursive loading attempted for " << identifier());
			return;
		}

		loading = true;
		try {
			if (loadAll())
				loaded = true;
		} catch (...) {
			loading = false;
			throw;
		}
		loading = false;
	}

	Named *NameSet::find(SimplePart *part) {
		if (Named *found = tryFind(part))
			return found;

		if (loaded)
			return null;

		if (!loadName(part))
			forceLoad();

		return tryFind(part);
	}

	Named *NameSet::tryFind(SimplePart *part) {
		if (!overloads->has(part->name))
			return false;

		NameOverloads *found = overloads->get(part->name);
		Named *result = part->choose(found);
		if (result)
			return result;

		return found->createTemplate(part);
	}

	Bool NameSet::loadName(SimplePart *part) {
		// Default implementation if the derived class does not support lazy-loading.
		// Report some matches may be found using 'loadAll'.
		return false;
	}

	Bool NameSet::loadAll() {
		// Default implementation if the derived class does not support lazy-loading.
		// Report done.
		return true;
	}

	void NameSet::toS(StrBuf *to) const {
		for (Overloads::Iter i = overloads->begin(); i != overloads->end(); ++i) {
			*to << i.v();
		}
	}

}
