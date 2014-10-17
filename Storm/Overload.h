#pragma once
#include "Value.h"
#include "Named.h"

namespace storm {

	/**
	 * An overloaded name. What differentiates two different
	 * names is the type and count of their operators.
	 * TODO? Inherit from Named as well?
	 */
	class NameOverload : public Named {
	public:
		// Give params.
		NameOverload(const String &name, const vector<Value> &params);

		// Parameters
		const vector<Value> params;
	};

	/**
	 * Representing a named entity consisting of (possibly) more than one
	 * entry, taking a different number of parameters.
	 */
	class Overload : public Named {
	public:
		Overload(const String &name);
		~Overload();

		// Add an overload.
		void add(NameOverload *n);

	protected:
		virtual void output(wostream &to) const;

	private:
		struct Item {
			Item(const vector<Value> &k);
			Item(NameOverload *overload);
			const vector<Value> &k;
			NameOverload *v;
			bool operator <(const Item &o) const;
		};

		typedef set<Item> ItemMap;
		ItemMap items;
	};

}
