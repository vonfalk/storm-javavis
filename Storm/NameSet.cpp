#include "stdafx.h"
#include "NameSet.h"
#include "Exception.h"
#include "Type.h"

namespace storm {

	NameOverloads::NameOverloads() {}

	Nat NameOverloads::count() const {
		return items.size();
	}

	Named *NameOverloads::operator [](Nat id) const {
		return items[id].ret();
	}

	Named *NameOverloads::at(nat id) const {
		return items[id].borrow();
	}

	void NameOverloads::add(Par<Named> item) {
		for (nat i = 0; i < items.size(); i++)
			if (items[i]->params == item->params)
				throw TypedefError(::toS(item) + L" is already defined as " + items[i]->identifier());

		items.push_back(item);
	}

	Named *NameOverloads::fromTemplate(Par<FoundParams> params) {
		if (!templ)
			return null;

		Auto<Named> named = templ->generate(params);
		if (!named)
			return null;

		assert(named->name == params->name, L"A template generated an unexpected name!");
		return named.ret();
	}

	void NameOverloads::output(wostream &to) const {
		for (nat i = 0; i < items.size(); i++)
			to << items[i] << endl;
		if (templ)
			to << L"<template>" << endl;
	}


	NameSet::NameSet(Par<Str> name) : Named(name) { init(); }
	NameSet::NameSet(const String &name) : Named(name) { init(); }
	NameSet::NameSet(const String &name, const vector<Value> &params) : Named(name, params) { init(); }

	void NameSet::init() {
		loaded = false;
		loading = false;
		nextAnon = 0;
	}

	NameSet::~NameSet() {
		clear();
	}

	void NameSet::clear() {
		overloads.clear();
		init();
	}

	void NameSet::add(Par<Named> p) {
		OverloadMap::iterator i = overloads.find(p->name);

		NameOverloads *o = null;
		if (i == overloads.end()) {
			o = CREATE(NameOverloads, this);
			overloads.insert(make_pair(p->name, steal(o)));
		} else {
			o = i->second.borrow();
		}

		o->add(p);
		p->parentLookup = this;
	}

	void NameSet::add(Par<Template> p) {
		OverloadMap::iterator i = overloads.find(p->name);

		NameOverloads *o = null;
		if (i == overloads.end()) {
			o = CREATE(NameOverloads, this);
			overloads.insert(make_pair(p->name, steal(o)));
		} else {
			o = i->second.borrow();
		}

		o->templ = p;
	}

	Str *NameSet::anonName() {
		String r = L"@ " + ::toS(nextAnon++);
		return CREATE(Str, this, r);
	}

	Named *NameSet::find(Par<FoundParams> part) {
		if (Named *found = tryFind(part))
			return found;

		if (loaded)
			return null;

		if (Named *found = loadName(part)) {
			add(found);
			return found;
		}

		forceLoad();
		return tryFind(part);
	}

	Named *NameSet::tryFind(Par<FoundParams> part) {
		OverloadMap::const_iterator i = overloads.find(part->name);
		if (i == overloads.end())
			return null;

		Auto<Named> result = part->choose(i->second);
		if (result)
			return result.ret();

		result = i->second->fromTemplate(part);
		if (result)
			add(result);
		return result.ret();
	}

	Named *NameSet::loadName(Par<FoundParams> part) {
		// Implemented in derived classes. Indicates 'not found'.
		return null;
	}

	bool NameSet::loadAll() {
		// Implemented in derived classes.
		return true;
	}

	void NameSet::forceLoad() {
		if (loaded)
			return;

		if (loading) {
			// This happens quite a lot...
			// WARNING(L"Recursive loading attempt of " << identifier());
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

	NameSet::iterator NameSet::begin() const {
		return iterator(overloads, overloads.begin(), 0);
	}

	NameSet::iterator NameSet::end() const {
		return iterator(overloads, overloads.end(), 0);
	}

	NameSet::iterator NameSet::begin(const String &name) const {
		OverloadMap::const_iterator found = overloads.find(name);
		return iterator(overloads, found, 0);
	}

	NameSet::iterator NameSet::end(const String &end) const {
		OverloadMap::const_iterator found = overloads.find(name);
		if (found != overloads.end())
			++found;
		return iterator(overloads, found, 0);
	}

	void NameSet::output(wostream &to) const {
		for (OverloadMap::const_iterator i = overloads.begin(), end = overloads.end(); i != end; ++i) {
			to << i->second;
		}
	}

	vector<Auto<Type>> NameSet::findTypes() const {
		vector<Auto<Type>> r;
		findTypes(r);
		return r;
	}

	void NameSet::findTypes(vector<Auto<Type>> &t) const {
		for (NameSet::iterator i = begin(), end = this->end(); i != end; ++i) {
			Named *n = i->borrow();
			if (Type *z = as<Type>(n)) {
				t.push_back(capture(z));
			}
			if (NameSet *z = as<NameSet>(n)) {
				z->findTypes(t);
			}
		}
	}

	ArrayP<Named> *NameSet::contents() {
		forceLoad();

		Auto<ArrayP<Named>> r = CREATE(ArrayP<Named>, this);
		for (iterator i = begin(), end = this->end(); i != end; ++i) {
			r->push(*i);
		}
		return r.ret();
	}

	void NameSet::compile() {
		forceLoad();

		for (iterator i = begin(), end = this->end(); i != end; ++i) {
			const Auto<Named> &named = *i;
			named->compile();
			os::UThread::leave();
		}
	}

	/**
	 * The iterator.
	 */

	NameSet::iterator::iterator() : m(null), pos(0) {}

	NameSet::iterator::iterator(const OverloadMap &m, OverloadMap::const_iterator i, nat pos) : m(&m), src(i), pos(pos) {}

	NameSet::iterator &NameSet::iterator::operator ++() {
		if (src->second->items.size() == ++pos) {
			do {
				++src;
				if (src == m->end())
					break;
			} while (src->second->items.size() == 0);
			pos = 0;
		}
		return *this;
	}

	NameSet::iterator NameSet::iterator::operator ++(int) {
		iterator old(*this);
		++(*this);
		return old;
	}

	NameSet::iterator &NameSet::iterator::operator --() {
		if (pos == 0) {
			do {
				if (src == m->begin())
					return *this;
				--src;
			} while (src->second->items.size() == 0);
			pos = src->second->items.size() - 1;
		} else {
			pos--;
		}
		return *this;
	}

	NameSet::iterator NameSet::iterator::operator --(int) {
		iterator old(*this);
		--(*this);
		return old;
	}

	bool NameSet::iterator::operator ==(const iterator &o) const {
		return (src == o.src) && (pos == o.pos);
	}

	bool NameSet::iterator::operator !=(const iterator &o) const {
		return !(*this == o);
	}

	Auto<Named> &NameSet::iterator::operator *() const {
		return src->second->items[pos];
	}

	Auto<Named> *NameSet::iterator::operator ->() const {
		return &src->second->items[pos];
	}

}
