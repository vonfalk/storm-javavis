#pragma once
#include "Value.h"
#include "Named.h"

namespace storm {

	/**
	 * An overloaded name. What differentiates two different
	 * names is the type and count of their operators.
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

		// Number of overloads.
		nat size() const { return items.size(); }

		// Get an overload, returns null on failure.
		NameOverload *find(const vector<Value> &p);

	protected:
		virtual void output(wostream &to) const;

	private:
		// Items here.
		vector<NameOverload *> items;

		// Is it a suitable overload?
		static bool suitable(NameOverload *overload, const vector<Value> &params);
	};

}
