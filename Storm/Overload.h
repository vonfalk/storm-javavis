#pragma once
#include "Value.h"
#include "Named.h"

namespace storm {
	class NameOverload;

	/**
	 * Representing a named entity consisting of (possibly) more than one
	 * entry, taking a different number of parameters.
	 */
	class Overload : public Named {
		STORM_CLASS;
	public:
		Overload(NameLookup *parent, const String &name);
		~Overload();

		// Add an overload.
		void add(NameOverload *n);

		// Number of overloads.
		nat size() const { return items.size(); }

		// Get an overload, returns null on failure.
		NameOverload *find(const vector<Value> &p);

		// Parent.
		NameLookup *parent() const;

	protected:
		virtual void output(wostream &to) const;

	private:
		// Items here.
		vector<Auto<NameOverload> > items;

		// Parent.
		NameLookup *p;

		// Is it a suitable overload?
		static bool suitable(const Auto<NameOverload> &overload, const vector<Value> &params);
	};


	/**
	 * An overloaded name. What differentiates two different
	 * names is the type and count of their operators.
	 */
	class NameOverload : public Named {
		friend class Overload;
		STORM_CLASS;
	public:
		// Give params.
		NameOverload(const String &name, const vector<Value> &params);

		// Parameters
		const vector<Value> params;

		// Human-readable identifier.
		virtual String identifier() const;

		// Owner.
		virtual Overload *parent() const;

		// Path.
		Name path() const;

	private:
		// Owner. Set by 'overload'.
		Overload * owner;
	};

}
