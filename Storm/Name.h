#pragma once
#include "Value.h"
#include "Lib/Object.h"
#include "Shared/Types.h"
#include "Shared/Array.h"
#include "Shared/CloneEnv.h"

namespace storm {
	STORM_PKG(core.lang);

	class Scope;
	class Named;
	class NameOverloads;
	class FoundParams;

	/**
	 * Represents one part of a name. Each part is a string and zero or more parameters (think
	 * templates in C++). Parameters may either be resolved names, or other names that has not yet
	 * been resolved. The base class represents the case when parameters are not neccessarily resolved.
	 */
	class NamePart : public Object {
		STORM_CLASS;
	public:
		// Copy.
		NamePart(Par<NamePart> o);

		// Create with just a name.
		NamePart(Par<Str> name);
		NamePart(const String &name);

		// Our name.
		const String name;

		// Empty/any?
		inline Bool STORM_FN empty() { return count() == 0; }
		Bool STORM_FN any() { return count() > 0; }

		// Parameter count.
		virtual Nat STORM_FN count();

		// Resolve names, NOTE: Not exposed to Storm as it has to be executed on the compiler thread.
		virtual MAYBE(FoundParams) *find(const Scope &scope);

		// Deep copy.
		virtual void STORM_FN deepCopy(Par<CloneEnv> env);

	protected:
		virtual void output(wostream &to) const;
	};

	// Create NameParts easily.
	NamePart *STORM_FN namePart(Par<Str> name);
	NamePart *STORM_FN namePart(Par<Str> name, Par<Array<Value>> params);
	NamePart *STORM_FN namePart(Par<Str> name, Par<ArrayP<Name>> params);

	// Wrappers.
	Str *STORM_FN name(Par<NamePart> part);

	/**
	 * A NamePart which has all parameters properly resolved to Values.
	 */
	class FoundParams : public NamePart {
		STORM_CLASS;
	public:
		// Copy ctor.
		STORM_CTOR FoundParams(Par<FoundParams> o);

		// Create with just a name.
		STORM_CTOR FoundParams(Par<Str> name);

		// Create with parameters as well.
		STORM_CTOR FoundParams(Par<Str> name, Par<Array<Value>> params);

		// C++ versions.
		FoundParams(const String &str);
	    FoundParams(const String &str, const vector<Value> &params);

		// Parameter count.
		virtual Nat STORM_FN count();

		// Get a parameter.
		Value STORM_FN operator [](Nat id);
		inline const Value &param(Nat id) const { return params[id]; }

		// Choose a specific Named from a set of overloads. Subclass and override to alter the
		// default behavior.
		virtual Named *STORM_FN choose(Par<NameOverloads> overloads);

		// Compute the badness of a candidate. Returns -1 on no match.
		virtual Int STORM_FN matches(Par<Named> candidate);

		// Resolve names.
		virtual MAYBE(FoundParams) *find(const Scope &scope);

		// Deep copy.
		virtual void STORM_FN deepCopy(Par<CloneEnv> env);

	protected:
		virtual void output(wostream &to) const;

		// Values. Note: We can not use Array<> here, as this class is used during early startup.
		vector<Value> params;
	};

	/**
	 * A NamePart which has unresolved parameters.
	 * TODO: Support for reference parameters.
	 */
	class NameParams : public NamePart {
		STORM_CLASS;
	public:
		// Copy.
		STORM_CTOR NameParams(Par<NameParams> o);

		// Create with just a name.
		STORM_CTOR NameParams(Par<Str> name);

		// Create with parameters as well.
		STORM_CTOR NameParams(Par<Str> name, Par<ArrayP<Name>> params);

		// C++ versions.
		NameParams(const String &name);
		NameParams(const String &name, const vector<Auto<Name>> params);

		// Parameter count.
		virtual Nat STORM_FN count();

		// Get a parameter.
		Name STORM_FN operator [](Nat id);

		// C++ version, returning borrowed ptr.
		inline Name *param(Nat id) const { return params[id].borrow(); }

		// Resolve names.
		virtual MAYBE(FoundParams) *find(const Scope &scope);

		// Deep copy.
		virtual void STORM_FN deepCopy(Par<CloneEnv> env);

	protected:
		virtual void output(wostream &to) const;

	private:
		// Values.
		vector<Auto<Name>> params;
	};


	// Storm wrappers.
	Str *STORM_FN name(Par<NamePart> p);
	Array<Value> *STORM_FN params(Par<NamePart> p);

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

		// One entry.
		STORM_CTOR Name(Par<Str> v);

		// One entry.
		STORM_CTOR Name(Par<Str> v, Par<Array<Value>> values);

		// Create with one entry.
		Name(const String &p, const vector<Value> &params = vector<Value>());

		// Copy ctor.
		STORM_CTOR Name(Par<const Name> n);

		// Append a new entry.
		void STORM_FN add(Par<NamePart> v);

		// Append a new simple entry for C++.
		void add(const String &name);
		void add(const String &name, const vector<Value> &params);
		void add(const String &name, const vector<Auto<Name>> &params);

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

		// Name of last element (convenience).
		const String &lastName() const;

		// Stick parameters to the last element.
		Name *withParams(const vector<Value> &v) const;

		// Replace the last element.
		Name *STORM_FN withLast(Par<NamePart> last) const;

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
		virtual Nat STORM_FN hash();

		// Deep copy.
		virtual void STORM_FN deepCopy(Par<CloneEnv> env);

	protected:
		virtual void output(std::wostream &to) const;

	private:
		// Store each part.
		vector<Auto<NamePart> > parts;
	};


	// Parse a name (does not support parameterized names).
	Name *parseSimpleName(Engine &e, const String &s);

	// Parse a name possibly containing template parameters (using <>)
	Name *parseTemplateName(Engine &e, const SrcPos &pos, const String &src);

}


