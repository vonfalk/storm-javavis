#pragma once
#include "Value.h"
#include "Named.h"

namespace storm {

	/**
	 * An overloaded name. What differentiates two different
	 * names is the type and count of their operators.
	 * TODO? Inherit from Named as well?
	 */
	class NameOverload : public Printable {
	public:
		// Give params.
		NameOverload(const String &name, const vector<Value> &params);

		const String name;
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

		// Our name.
		const String &name;

		// Add an overload.
		void add(NameOverload *n);

	protected:
		virtual void output(wostream &to) const;

	private:
		struct Item {
			Item(NameOverload *overload);
			NameOverload *v;
			bool operator <(const Item &o) const;
		};

		typedef set<Item> ItemMap;
		ItemMap items;
	};

}
