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
	class SimplePart;
	class SimpleName;

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
		virtual MAYBE(SimplePart) *find(const Scope &scope);

		// Deep copy.
		virtual void STORM_FN deepCopy(Par<CloneEnv> env);

	protected:
		virtual void output(wostream &to) const;
	};

	// Create Parts easily.
	NamePart *STORM_FN namePart(Par<Str> name);
	NamePart *STORM_FN namePart(Par<Str> name, Par<Array<Value>> params);
	NamePart *STORM_FN namePart(Par<Str> name, Par<ArrayP<Name>> params);

	// Wrappers.
	Str *STORM_FN name(Par<NamePart> part);

	/**
	 * A NamePart which has all parameters properly resolved to Values.
	 */
	class SimplePart : public NamePart {
		STORM_CLASS;
	public:
		// Copy ctor.
		STORM_CTOR SimplePart(Par<SimplePart> o);

		// Create with just a name.
		STORM_CTOR SimplePart(Par<Str> name);

		// Create with parameters as well.
		STORM_CTOR SimplePart(Par<Str> name, Par<Array<Value>> params);

		// C++ versions.
		SimplePart(const String &str);
	    SimplePart(const String &str, const vector<Value> &params);

		// Parameter count.
		virtual Nat STORM_FN count();

		// Get a parameter.
		Value STORM_FN operator [](Nat id);
		inline const Value &param(Nat id) const { return data[id]; }

		// Choose a specific Named from a set of overloads. Subclass and override to alter the
		// default behavior.
		virtual Named *STORM_FN choose(Par<NameOverloads> overloads);

		// Compute the badness of a candidate. Returns -1 on no match.
		virtual Int STORM_FN matches(Par<Named> candidate);

		// Resolve names.
		virtual MAYBE(SimplePart) *find(const Scope &scope);

		// Get parameters.
		Array<Value> *STORM_FN params();

		// Deep copy.
		virtual void STORM_FN deepCopy(Par<CloneEnv> env);

	protected:
		virtual void output(wostream &to) const;

		// Values. Note: We can not use Array<> here, as this class is used during early startup.
		vector<Value> data;
	};

	/**
	 * A NamePart which has unresolved (recursive) parameters.
	 * TODO: Support for reference parameters.
	 */
	class RecNamePart : public NamePart {
		STORM_CLASS;
	public:
		// Copy.
		STORM_CTOR RecNamePart(Par<RecNamePart> o);

		// Create with just a name.
		STORM_CTOR RecNamePart(Par<Str> name);

		// Create with parameters as well.
		STORM_CTOR RecNamePart(Par<Str> name, Par<ArrayP<Name>> params);

		// C++ versions.
		RecNamePart(const String &name);
		RecNamePart(const String &name, const vector<Auto<Name>> params);

		// Parameter count.
		virtual Nat STORM_FN count();

		// Get a parameter.
		inline Name *STORM_FN operator [](Nat id) { return params[id].borrow(); }

		// C++ version, returning borrowed ptr.
		inline Name *param(Nat id) const { return params[id].borrow(); }

		// Resolve names.
		virtual MAYBE(SimplePart) *find(const Scope &scope);

		// Deep copy.
		virtual void STORM_FN deepCopy(Par<CloneEnv> env);

	protected:
		virtual void output(wostream &to) const;

	private:
		// Values.
		vector<Auto<Name>> params;
	};


	/**
	 * Representation of a name, either a relative name or an absolute
	 * name including the full package path.
	 *
	 * TODO: Strip some operations from here!
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

		// Convert from a SimpleName.
		STORM_CTOR Name(Par<SimpleName> simple);

		// Create with one entry.
		Name(const String &p, const vector<Value> &params = vector<Value>());

		// Copy ctor.
		STORM_CTOR Name(Par<const Name> n);

		// Append a new entry.
		void STORM_FN add(Par<NamePart> v);
		void STORM_FN add(Par<Str> v);
		void STORM_FN add(Par<Str> name, Par<Array<Value>> v);
		void STORM_FN add(Par<Str> name, Par<ArrayP<Name>> v);

		// Append a new simple entry for C++.
		void add(const String &name);
		void add(const String &name, const vector<Value> &params);
		void add(const String &name, const vector<Auto<Name>> &params);

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
		inline Bool STORM_FN root() const { return count() == 0; }

		// Access to individual elements.
		inline Nat count() const { return parts.size(); }
		inline NamePart *operator [](Nat id) const { Auto<NamePart> r = parts[id]; return r.ret(); }
		inline NamePart *at(Nat id) const { return parts[id].borrow(); }

		// empty/any
		inline Bool STORM_FN any() const { return count() > 0; }
		inline Bool STORM_FN empty() const { return count() == 0; }

		// Create a SimpleName from this name. Not exposed to Storm due to threading.
		MAYBE(SimpleName) *simplify(const Scope &scope);

		// Deep copy.
		virtual void STORM_FN deepCopy(Par<CloneEnv> env);

	protected:
		virtual void output(std::wostream &to) const;

	private:
		// Store each part.
		vector<Auto<NamePart>> parts;
	};


	// Parse a name possibly containing template parameters (using <>)
	Name *parseTemplateName(Engine &e, const SrcPos &pos, const String &src);


	/**
	 * A name that only contains resolved parts. Created by Scopes when starting to resolve a
	 * regular name.
	 */
	class SimpleName : public Object {
		STORM_CLASS;
	public:
		STORM_CTOR SimpleName();
		STORM_CTOR SimpleName(Par<SimpleName> name);
		STORM_CTOR SimpleName(Par<Str> part);
		SimpleName(const String &part);
		SimpleName(const String &part, const vector<Value> &params);

		// Add a part.
		void STORM_FN add(Par<SimplePart> part);
		void STORM_FN add(Par<Str> v);
		void add(const String &name);
		void add(const String &name, const vector<Value> &params);

		// Number of parts.
		inline Nat STORM_FN count() const { return parts.size(); }

		// Get a part (for C++).
		inline const Auto<SimplePart> &at(Nat id) const { return parts[id]; }
		inline Auto<SimplePart> &at(Nat id) { return parts[id]; }

		// Get a part (for Storm).
		inline Auto<SimplePart> &STORM_FN operator [](Nat id) { return parts[id]; }

		// Empty/any?
		inline Bool STORM_FN any() const { return count() > 0; }
		inline Bool STORM_FN empty() const { return count() == 0; }

		// Last element.
		inline Auto<SimplePart> &STORM_FN last() { return parts.back(); }

		// Name of last element.
		const String &lastName() const;

		// Get a SimpleName with our contents from the 'n'th element.
		SimpleName *STORM_FN from(Nat id) const;

		// Deep copy.
		virtual void STORM_FN deepCopy(Par<CloneEnv> env);

		// Equality and hash.
		bool operator ==(const SimpleName &o) const;
		inline bool operator !=(const SimpleName &o) const { return !(*this == o); }
		virtual Nat STORM_FN hash();

	protected:
		virtual void output(std::wostream &to) const;

	private:
		// Store each part.
		vector<Auto<SimplePart>> parts;
	};

	// Parse a name.
	SimpleName *parseSimpleName(Engine &e, const String &s);

}


