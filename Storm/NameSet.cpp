#include "stdafx.h"
#include "NameSet.h"
#include "Exception.h"

namespace storm {

	NameSet::NameSet(Par<Str> name) : Named(name) {}
	NameSet::NameSet(const String &name) : Named(name) {}
	NameSet::NameSet(const String &name, const vector<Value> &params) : Named(name, params) {}

	NameSet::~NameSet() {
		clear();
	}

	void NameSet::clear() {
		clearMap(overloads);
	}

	NameSet::Overload::Overload() {}

	NameSet::Overload::~Overload() {}

	void NameSet::add(Par<Named> p) {
		Overload *o = null;
		OverloadMap::iterator i = overloads.find(p->name);

		if (i == overloads.end())
			o = new Overload();
		else
			o = i->second;

		add(o, p);
	}

	Named *NameSet::find(Par<NamePart> name) const {
		return find(name->name, name->params);
	}

	Named *NameSet::find(const String &name, const vector<Value> &params) const {
		OverloadMap::const_iterator i = overloads.find(name);
		if (i == overloads.end())
			return null;

		return find(i->second, params);
	}

	void NameSet::add(Overload *to, Par<Named> n) {
		if (find(to, n->params))
			throw TypedefError(::toS(n) + L" is already defined in " + identifier());

		to->items.push_back(n);
		n->parentLookup = this;
	}

	Named *NameSet::find(Overload *from, const vector<Value> &params) const {
		for (nat i = 0; i < from->items.size(); i++) {
			Named *c = from->items[i].borrow();
			if (candidate(c->params, params))
				return c;
		}
		return null;
	}

	bool NameSet::candidate(const vector<Value> &our, const vector<Value> &ref) const {
		nat params = our.size();
		// No support for default-parameters here!
		if (params != ref.size())
			return false;

		for (nat i = 0; i < params; i++)
			if (!our[i].canStore(ref[i]))
				return false;

		return true;

	}

	NameSet::iterator NameSet::begin() const {
		return iterator(overloads.begin(), 0);
	}

	NameSet::iterator NameSet::end() const {
		return iterator(overloads.end(), 0);
	}

	NameSet::iterator NameSet::begin(const String &name) const {
		OverloadMap::const_iterator found = overloads.find(name);
		return iterator(found, 0);
	}

	NameSet::iterator NameSet::end(const String &end) const {
		OverloadMap::const_iterator found = overloads.find(name);
		if (found != overloads.end())
			++found;
		return iterator(found, 0);
	}

	void NameSet::output(wostream &to) const {
		for (iterator i = begin(); i != end(); ++i)
			to << *i;
	}

	/**
	 * The iterator.
	 */

	NameSet::iterator::iterator() : pos(0) {}

	NameSet::iterator::iterator(OverloadMap::const_iterator i, nat pos) : src(i), pos(pos) {}

	NameSet::iterator &NameSet::iterator::operator ++() {
		if (src->second->items.size() == ++pos) {
			++src;
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
			--src;
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
