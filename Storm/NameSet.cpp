#include "stdafx.h"
#include "NameSet.h"
#include "Exception.h"
#include "Type.h"

namespace storm {

	NameSet::NameSet(Par<Str> name) : Named(name) { init(); }
	NameSet::NameSet(const String &name) : Named(name) { init(); }
	NameSet::NameSet(const String &name, const vector<Value> &params) : Named(name, params) { init(); }

	void NameSet::init() {
		loaded = false;
		loading = false;
	}

	NameSet::~NameSet() {
		clear();
	}

	void NameSet::clear() {
		clearMap(overloads);
		init();
	}

	NameSet::Overload::Overload() {}

	NameSet::Overload::~Overload() {}

	void NameSet::add(Par<Named> p) {
		OverloadMap::iterator i = overloads.find(p->name);

		Overload *o = null;
		if (i == overloads.end()) {
			o = new Overload();
			overloads.insert(make_pair(p->name, o));
		} else {
			o = i->second;
		}

		add(o, p);
	}

	void NameSet::add(Par<Template> p) {
		OverloadMap::iterator i = overloads.find(p->name);

		Overload *o = null;
		if (i == overloads.end()) {
			o = new Overload();
			overloads.insert(make_pair(p->name, o));
		} else {
			o = i->second;
		}

		o->templ = p;
	}

	Named *NameSet::findHere(const String &name, const vector<Value> &params) {
		if (Named *found = tryFind(name, params))
			return found;

		if (loaded)
			return null;

		if (Named *found = loadName(name, params)) {
			add(found);
			found->release();
			return found;
		}

		forceLoad();
		return tryFind(name, params);
	}

	Named *NameSet::tryFind(const String &name, const vector<Value> &params) {
		OverloadMap::const_iterator i = overloads.find(name);
		if (i == overloads.end())
			return null;

		Overload *o = i->second;
		if (Named *n = tryFind(o, params))
			return n;

		if (o->templ) {
			Auto<NamePart> part = CREATE(NamePart, this, name, params);
			if (Auto<Named> n = o->templ->generate(part)) {
				assert(n->name == name, L"The template " + o->templ->name + L" returned a mismatched Name: " + n->name);
				if (n->name != name)
					return null;
				add(n);
				return n.borrow();
			}
		}

		return null;
	}

	Named *NameSet::tryFind(Overload *from, const vector<Value> &params) {
		for (nat i = 0; i < from->items.size(); i++) {
			Named *c = from->items[i].borrow();
			if (candidate(c->matchFlags, c->params, params))
				return c;
		}
		return null;
	}

	bool NameSet::candidate(MatchFlags flags, const vector<Value> &our, const vector<Value> &ref) const {
		nat params = our.size();
		// No support for default-parameters here!
		if (params != ref.size())
			return false;

		for (nat i = 0; i < params; i++)
			if (!our[i].matches(ref[i], flags))
				return false;

		return true;
	}

	void NameSet::add(Overload *to, Par<Named> n) {
		if (tryFind(to, n->params)) {
			DebugBreak();
			throw TypedefError(::toS(n) + L" is already defined in " + identifier());
		}

		to->items.push_back(n);
		n->parentLookup = this;
	}

	Named *NameSet::loadName(const String &name, const vector<Value> &params) {
		// Implemented in derived classes. Indicate 'not found'.
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
		for (iterator i = begin(); i != end(); ++i)
			to << *i << endl;
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
			} else if (NameSet *z = as<NameSet>(n)) {
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
