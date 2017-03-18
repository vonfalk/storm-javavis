#pragma once
#include "NamePart.h"
#include "SrcPos.h"

namespace storm {
	STORM_PKG(core.lang);

	class SimpleName;

	/**
	 * Representation of a name, either a relative name or an absolute name.
	 */
	class Name : public Object {
		STORM_CLASS;
	public:
		// Path to the root package.
		STORM_CTOR Name();

		// Create with one entry.
		STORM_CTOR Name(NamePart *v);
		STORM_CTOR Name(Str *name);
		STORM_CTOR Name(Str *name, Array<Value> *params);
		STORM_CTOR Name(Str *name, Array<Name> *params);

		// Convert form a SimpleName.
		STORM_CAST_CTOR Name(SimpleName *simple);

		// Copy.
		Name(const Name &o);

		// Deep copy.
		virtual void STORM_FN deepCopy(CloneEnv *env);

		// Append a new entry.
		void STORM_FN add(NamePart *v);
		void STORM_FN add(Str *name);
		void STORM_FN add(Str *name, Array<Value> *params);
		void STORM_FN add(Str *name, Array<Name> *params);

		// Get the parent name.
		Name *STORM_FN parent() const;

		// Get the last element.
		NamePart *STORM_FN last() const { return parts->last(); }

		// Number of elements.
		Nat STORM_FN count() const { return parts->count(); }

		// Access elements.
		NamePart *STORM_FN operator [](Nat i) const { return parts->at(i); }
		NamePart *at(Nat i) const { return parts->at(i); }

		// Any/empty.
		Bool STORM_FN any() const { return parts->any(); }
		Bool STORM_FN empty() const { return parts->empty(); }

		// Create a SimpleName from this name. Not exposed to Storm due to theading.
		MAYBE(SimpleName *) simplify(const Scope &scope);

		// ToS.
		virtual void STORM_FN toS(StrBuf *to) const;

	private:
		// Data.
		Array<NamePart *> *parts;
	};

	/**
	 * Name with a SrcPos attached.
	 */
	class SrcName : public Name {
		STORM_CLASS;
	public:
		STORM_CTOR SrcName();
		STORM_CTOR SrcName(SrcPos pos);
		STORM_CTOR SrcName(Name *o, SrcPos pos);
		STORM_CTOR SrcName(SimpleName *o, SrcPos pos);

		SrcPos pos;

		virtual void STORM_FN deepCopy(CloneEnv *env);
	};

	/**
	 * A name which only contains resolved parts. Created by Scopes when starting to resolve a
	 * regular name.
	 */
	class SimpleName : public Object {
		STORM_CLASS;
	public:
		// Path to the root package.
		STORM_CTOR SimpleName();

		// Create with one entry.
		STORM_CTOR SimpleName(SimplePart *v);
		STORM_CTOR SimpleName(Str *name);
		STORM_CTOR SimpleName(Str *name, Array<Value> *params);

		// Copy.
		SimpleName(const SimpleName &simple);

		// Deep copy.
		virtual void STORM_FN deepCopy(CloneEnv *env);

		// Append a new entry.
		void STORM_FN add(SimplePart *v);
		void STORM_FN add(Str *name);
		void STORM_FN add(Str *name, Array<Value> *params);

		// Get the parent name.
		SimpleName *STORM_FN parent() const;

		// Get the last element.
		SimplePart *STORM_FN last() const { return parts->last(); }
		SimplePart *&last() { return parts->last(); }

		// Number of elements.
		Nat STORM_FN count() const { return parts->count(); }

		// Access elements.
		SimplePart *STORM_FN operator [](Nat i) const { return parts->at(i); }
		SimplePart *at(Nat i) const { return parts->at(i); }

		// Any/empty.
		Bool STORM_FN any() const { return parts->any(); }
		Bool STORM_FN empty() const { return parts->empty(); }

		// Get all elements starting from 'n'.
		SimpleName *STORM_FN from(Nat id) const;

		// ToS.
		virtual void STORM_FN toS(StrBuf *to) const;

		// Equals.
		virtual Bool STORM_FN equals(Object *o) const;

		// Hash.
		virtual Nat STORM_FN hash() const;

		inline void *debug() const { return parts; }
	private:
		// Data.
		Array<SimplePart *> *parts;
	};

	// Parse a string containing a dot-separated name.
	SimpleName *STORM_FN parseSimpleName(Str *str);
	SimpleName *parseSimpleName(Engine &e, const wchar *str);

}
