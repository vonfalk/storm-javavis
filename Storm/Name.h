#pragma once
#include "Value.h"
#include "Lib/Types.h"

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * Represents one part of a name. Each part is a string and zero or more
	 * parameters (think templates in C++).
	 */
	class NamePart : public Object {
		STORM_CLASS;
	public:
		// Create with only a name.
		STORM_CTOR NamePart(Par<Str> name);

		// Create from C++.
		NamePart(const String &name);

		// Create with params.
		NamePart(const String &name, const vector<Value> &params);

		// The name here.
		const String name;

		// Any parameters present.
		const vector<Value> params;

		// Equality check.
		bool operator ==(const NamePart &o) const;
		inline bool operator !=(const NamePart &o) const { return !(*this == o); }

	protected:
		// Output.
		virtual void output(std::wostream &to) const;
	};

	/**
	 * Representation of a name, either a relative name or an absolute
	 * name including the full package path.
	 */
	class Name : public Object {
		STORM_CLASS;
	public:
		// Path to the root package.
		STORM_CTOR Name();

		// Create with one entry.
		STORM_CTOR Name(Par<NamePart> v);

		// Create with one entry.
		Name(const String &p);

		// Copy ctor.
		STORM_CTOR Name(Par<const Name> n);

		// Append a new entry.
		void STORM_FN add(Par<NamePart> v);

		// Append a new simple entry for C++.
		void add(const String &name);
		void add(const String &name, const vector<Value> &params);

		// Concat paths.
		// Name operator +(const Name &o) const;
		// Name operator +=(const Name &o) const;


		// Equality.
		bool operator ==(const Name &o) const;
		inline bool operator !=(const Name &o) const { return !(*this == o); }

		// Ordering.
		// inline bool operator <(const Name &o) const { return parts < o.parts; }
		// inline bool operator >(const Name &o) const { return parts > o.parts; }

		// Get the parent.
		Name *STORM_FN parent() const;

		// Last element.
		NamePart *STORM_FN last() const;

		// Stick parameters to the last element.
		Name *withParams(const vector<Value> &v) const;

		// All elements from 'n'.
		Name *STORM_FN from(Nat n) const;

		// Is this the root package?
		inline Bool STORM_FN root() const { return size() == 0; }

		// Access to individual elements (BORROWED PTR, not exposed to storm).
		// TODO: Expose to Storm!
		inline Nat size() const { return parts.size(); }
		inline NamePart *at(Nat id) const { return parts[id].borrow(); }

		// empty/any
		inline Bool STORM_FN any() const { return size() > 0; }
		inline Bool STORM_FN empty() const { return size() == 0; }

		// Hash function.
		size_t hash() const;

	protected:
		virtual void output(std::wostream &to) const;

	private:
		// Store each part.
		vector<Auto<NamePart> > parts;
	};


	// Parse a name (does not support parameterized names).
	Name *parseSimpleName(Engine &e, const String &s);

}


