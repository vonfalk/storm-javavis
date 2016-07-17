#pragma once
#include "CppName.h"
#include "SrcPos.h"
#include "Auto.h"
#include "Exception.h"

/**
 * A map from C++ names into some type. Acts like both a searchable map as well as an ordered array.
 *
 * Assumes T::name, T::pos and T::id exists. Auto<T> has to be usable.
 */
template <class T>
class NameMap {
public:
	NameMap(const vector<CppName> &usingExpr) : usingExpr(usingExpr) {}

	// Insert a new element at the end.
	void insert(const Auto<T> &v) {
		if (lookup.count(v->name))
			throw Error(L"The name " + toS(v->name) + L" is defined twice!", v->pos);

		lookup.insert(make_pair(v->name, order.size()));
		order.push_back(v);
	}

	// Order the types according to a predicate.
	template <class Pred>
	void sort(const Pred &pred) {
		std::sort(order.begin(), order.end(), pred);

		for (nat i = 0; i < order.size(); i++) {
			order[i]->id = i;
			lookup[order[i]->name] = i;
		}
	}

	// Total number of elements.
	nat size() const {
		return order.size();
	}

	// Access elements.
	const Auto<T> &operator[] (nat id) const {
		return order[id];
	}

	// Find by name.
	T *findUnsafe(const CppName &name, CppName context) const {
		map<CppName, nat>::const_iterator i = lookup.find(name);
		if (i != lookup.end())
			return order[i->second].borrow();

		while (!context.empty()) {
			i = lookup.find(context + name);
			if (i != lookup.end())
				return order[i->second].borrow();

			context = context.parent();
		}

		// Reachable via 'using namespace'?
		for (nat j = 0; j < usingExpr.size(); j++) {
			i = lookup.find(usingExpr[j] + name);
			if (i != lookup.end())
				return order[i->second].borrow();
		}

		return null;
	}

	// Find by name, throws exception if not found.
	T *find(const CppName &name, CppName context, const SrcPos &pos) const {
		T *t = findUnsafe(name, context);
		if (!t)
			throw Error(L"The name " + name + L" is not known to Storm.", pos);
		return t;
	}


private:
	// All using-expressions.
	const vector<CppName> &usingExpr;

	// Quick lookups.
	map<CppName, nat> lookup;

	// Order.
	vector<Auto<T>> order;
};

