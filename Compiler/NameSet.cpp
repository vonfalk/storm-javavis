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

	Bool NameOverloads::empty() const {
		return items->empty()
			&& templates->empty();
	}

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

		for (Nat i = 0; i < a->count(); i++)
			if (a->at(i) != b->at(i))
				return false;

		return true;
	}

	Bool NameOverloads::has(Named *item) {
		for (Nat i = 0; i < items->count(); i++) {
			if (storm::equals(item->params, items->at(i)->params))
				return true;
		}
		return false;
	}

	void NameOverloads::add(Named *item) {
		for (Nat i = 0; i < items->count(); i++) {
			if (storm::equals(item->params, items->at(i)->params)) {
				throw new (this) TypedefError(
					item->pos,
					TO_S(engine(), item << S(" is already defined as ") << items->at(i)->identifier()));
			}
		}

		items->push(item);
	}

	void NameOverloads::add(Template *item) {
		// There is no way to validate templates at this stage.
		templates->push(item);
	}

	Bool NameOverloads::remove(Named *item) {
		for (Nat i = 0; i < items->count(); i++) {
			if (items->at(i) == item) {
				items->remove(i);
				return true;
			}
		}

		return false;
	}

	Bool NameOverloads::remove(Template *item) {
		for (Nat i = 0; i < templates->count(); i++) {
			if (templates->at(i) == item) {
				templates->remove(i);
				return true;
			}
		}

		return false;
	}

	void NameOverloads::merge(NameOverloads *from) {
		// Validate first.
		for (Nat i = 0; i < from->items->count(); i++) {
			Named *add = from->items->at(i);

			for (Nat j = 0; j < items->count(); j++) {
				PLN(L"Checking " << (void *)add << L", " << (void *)items->at(j));
				if (storm::equals(add->params, items->at(j)->params)) {
					throw new (this) TypedefError(
						add->pos,
						TO_S(engine(), add << S(" is already defined as ") << items->at(j)->identifier()));
				}
			}
		}

		for (Nat i = 0; i < from->templates->count(); i++)
			templates->push(from->templates->at(i));

		for (Nat i = 0; i < from->items->count(); i++)
			items->push(from->items->at(i));
	}

	Named *NameOverloads::createTemplate(NameSet *owner, SimplePart *part) {
		Named *found = null;
		for (Nat i = 0; i < templates->count(); i++) {
			Named *n = templates->at(i)->generate(part);
			if (found != null && n != null) {
				throw new (this) TypedefError(owner->pos, TO_S(engine(), S("Multiple template matches for: ") << part));
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
		for (Nat i = 0; i < items->count(); i++)
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
		nextAnon = 0;

		if (engine().has(bootTemplates))
			lateInit();
	}

	void NameSet::lateInit() {
		Named::lateInit();

		if (!overloads)
			overloads = new (this) Map<Str *, NameOverloads *>();
	}

	void NameSet::compile() {
		forceLoad();

		for (Iter i = begin(), e = end(); i != e; ++i)
			i.v()->compile();
	}

	void NameSet::discardSource() {
		for (Iter i = begin(), e = end(); i != e; ++i)
			i.v()->discardSource();
	}

	void NameSet::watchAdd(Named *notifyTo) {
		if (!notify)
			notify = new (this) WeakSet<Named>();
		notify->put(notifyTo);
	}

	void NameSet::watchRemove(Named *notifyTo) {
		if (!notify)
			return;
		notify->remove(notifyTo);
	}

	void NameSet::notifyAdd(Named *what) {
		if (!notify)
			return;

		WeakSet<Named>::Iter i = notify->iter();
		while (Named *n = i.next()) {
			n->notifyAdded(this, what);
		}
	}

	void NameSet::notifyRemove(Named *what) {
		if (!notify)
			return;

		WeakSet<Named>::Iter i = notify->iter();
		while (Named *n = i.next()) {
			n->notifyRemoved(this, what);
		}
	}

	Bool NameSet::has(Named *item) const {
		if (!overloads)
			return false;

		return overloads->at(item->name)->has(item);
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

	Bool NameSet::remove(Named *item) {
		if (!overloads)
			return false;

		NameOverloads *o = overloads->at(item->name);
		Bool ok = o->remove(item);
		if (ok)
			notifyRemove(item);
		if (o->empty())
			overloads->remove(item->name);
		return ok;
	}

	Bool NameSet::remove(Template *item) {
		if (!overloads)
			return false;

		NameOverloads *o = overloads->at(item->name);
		Bool ok = o->remove(item);
		if (o->empty())
			overloads->remove(item->name);
		return false;
	}

	Str *NameSet::anonName() {
		StrBuf *buf = new (this) StrBuf();
		*buf << S("@ ") << (nextAnon++);
		return buf->toS();
	}

	Array<Named *> *NameSet::content() {
		Array<Named *> *r = new (this) Array<Named *>();
		for (Overloads::Iter at = overloads->begin(); at != overloads->end(); ++at) {
			NameOverloads &o = *at.v();
			for (Nat i = 0; i < o.count(); i++)
				r->push(o[i]);
		}
		return r;
	}

	void NameSet::forceLoad() {
		if (loaded)
			return;

		if (loading) {
			// This happens quite a lot...
			// WARNING(L"Recursive loading attempted for " << name);
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

	Named *NameSet::find(SimplePart *part, Scope source) {
		if (Named *found = tryFind(part, source))
			return found;

		if (loaded)
			return null;

		if (!loadName(part))
			forceLoad();

		return tryFind(part, source);
	}

	Named *NameSet::tryFind(SimplePart *part, Scope source) {
		if (!overloads)
			return null;

		Overloads::Iter i = overloads->find(part->name);
		if (i == overloads->end())
			return null;

		NameOverloads *found = i.v();
		Named *result = part->choose(found, source);
		if (result)
			return result;

		Named *t = found->createTemplate(this, part);
		// Are we allowed to access the newly created template?
		if (!t || !t->visibleFrom(source))
			return null;

		return t;
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

	void NameSet::merge(NameSet *from) {
		if (!overloads)
			overloads = new (this) Map<Str *, NameOverloads *>();

		for (Overloads::Iter i = from->overloads->begin(), end = from->overloads->end(); i != end; ++i) {
			overloads->at(i.k())->merge(i.v());
		}
	}

	NameSet::Iter::Iter() : name(), pos(0), nextSet(null) {}

	NameSet::Iter::Iter(Map<Str *, NameOverloads *> *c, NameSet *next) : name(c->begin()), pos(0), nextSet(next) {
		advance();
	}

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

	void NameSet::Iter::advance() {
		while (name != MapIter() && pos >= name.v()->count()) {
			++name;
			pos = 0;
		}

		if (nextSet && name == MapIter())
			*this = nextSet->begin();
	}

	NameSet::Iter &NameSet::Iter::operator ++() {
		pos++;
		advance();
		return *this;
	}

	NameSet::Iter NameSet::Iter::operator ++(int) {
		Iter i(*this);
		++*this;
		return i;
	}

	NameSet::Iter NameSet::begin() const {
		if (overloads)
			return Iter(overloads, null);
		else
			return Iter();
	}

	NameSet::Iter NameSet::begin(NameSet *after) const {
		if (overloads)
			return Iter(overloads, after);
		else
			return after->begin();
	}

	NameSet::Iter NameSet::end() const {
		return Iter();
	}

	Array<Named *> *NameSet::findName(Str *name) const {
		Array<Named *> *result = new (this) Array<Named *>();

		if (NameOverloads *f = overloads->get(name, null)) {
			result->reserve(f->count());
			for (Nat i = 0; i < f->count(); i++)
				result->push(f->at(i));
		}

		return result;
	}

	void NameSet::dbg_dump() const {
		PLN(L"Name set:");
		for (Overloads::Iter i = overloads->begin(); i != overloads->end(); ++i) {
			PLN(L" " << i.k() << L":");
			NameOverloads *o = i.v();
			for (Nat i = 0; i < o->count(); i++) {
				PLN(L"  " << o->at(i)->identifier());
			}
		}
	}

}
