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
			if (storm::equals(item->params, items->at(i)->params)) {
				throw TypedefError(::toS(item) + L" is already defined as " + ::toS(items->at(i)->identifier()));
			}

		items->push(item);
	}

	void NameOverloads::add(Template *item) {
		// There is no way to validate templates at this stage.
		templates->push(item);
	}

	Named *NameOverloads::createTemplate(NameSet *owner, SimplePart *part) {
		Named *found = null;
		for (nat i = 0; i < templates->count(); i++) {
			Named *n = templates->at(i)->generate(part);
			if (found != null && n != null) {
				throw TypedefError(L"Multiple template matches for: " + ::toS(part));
			} else if (n) {
				found = n;
			}
		}

		if (found) {
			// TODO: Go through the regular 'add' of the NameSet!
			add(found);
			found->parentLookup = owner;
		}
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

	void NameSet::watchAdd(Named *notifyTo) {
		if (!notify)
			notify = new (this) WeakSet<Named>();
		notify->put(notifyTo);
	}

	void NameSet::notifyAdd(Named *what) {
		if (!notify)
			return;

		WeakSet<Named>::Iter i = notify->iter();
		while (Named *n = i.next()) {
			n->notifyAdded(this, what);
		}
	}

	void NameSet::add(Named *item) {
		if (!overloads)
			overloads = new (this) Map<Str *, NameOverloads *>();

		overloads->at(item->name)->add(item);
		item->parentLookup = this;
		notifyAdd(item);
	}

	void NameSet::add(Template *item) {
		if (!overloads)
			overloads = new (this) Map<Str *, NameOverloads *>();

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
		if (!overloads)
			return null;

		if (!overloads->has(part->name))
			return null;

		NameOverloads *found = overloads->get(part->name);
		Named *result = part->choose(found);
		if (result)
			return result;

		return found->createTemplate(this, part);
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

	NameSet::Iter::Iter() : name(), pos(0) {}

	NameSet::Iter::Iter(Map<Str *, NameOverloads *> *c) : name(c->begin()), pos(0) {}

	Bool NameSet::Iter::operator ==(const Iter &o) const {
		// Either both at end or none.
		if (name == MapIter())
			return o.name == MapIter();

		if (name != o.name)
			return false;

		return pos == o.pos;
	}

	Bool NameSet::Iter::operator !=(const Iter &o) const {
		return !(*this == o);
	}

	Named *NameSet::Iter::v() const {
		return name.v()->at(pos);
	}

	NameSet::Iter &NameSet::Iter::operator ++() {
		while (name != MapIter() && pos >= name.v()->count()) {
			++name;
			pos = 0;
		}

		return *this;
	}

	NameSet::Iter NameSet::Iter::operator ++(int) {
		Iter i(*this);
		++*this;
		return i;
	}

	NameSet::Iter NameSet::begin() const {
		Iter r;
		if (overloads)
			r = Iter(overloads);
		return r;
	}

	NameSet::Iter NameSet::end() const {
		return Iter();
	}

}
