#pragma once
#include "Core/Array.h"
#include "Value.h"
#include "Syntax/SStr.h"

namespace storm {
	STORM_PKG(core.lang);

	class SimplePart;
	class Scope;
	class Name;
	class Named;
	class NameOverloads;
	class NameLookup;

	/**
	 * Represents one part of a name. Each part is a string and zero or more parameters (think
	 * templates in C++). Parameters may either be resolved names, or other names that has not yet
	 * been resolved. The base class represents the case when parameters are not neccessarily resolved.
	 */
	class NamePart : public Object {
		STORM_CLASS;
	public:
		// Create with a single name.
		STORM_CTOR NamePart(Str *name);
		NamePart(const wchar *name);

		// Deep copy.
		virtual void STORM_FN deepCopy(CloneEnv *env);

		// Our name.
		Str *name;

		// Resolve names.
		virtual MAYBE(SimplePart *) find(const Scope &scope) ON(Compiler);

		// To string.
		virtual void STORM_FN toS(StrBuf *to) const;
	};

	/**
	 * A NamePart which has all parameters properly resolved to Values.
	 */
	class SimplePart : public NamePart {
		STORM_CLASS;
	public:
		// Create with just a name.
		STORM_CTOR SimplePart(Str *name);
		STORM_CTOR SimplePart(syntax::SStr *name);
		SimplePart(const wchar *name);

		// Create with name and parameters.
		STORM_CTOR SimplePart(Str *name, Array<Value> *params);

		// Create with name and one parameter.
		STORM_CTOR SimplePart(Str *name, Value param);

		// Deep copy.
		virtual void STORM_FN deepCopy(CloneEnv *env);

		// Parameters.
		Array<Value> *params;

		// Resolve names.
		virtual MAYBE(SimplePart *) find(const Scope &scope);

		// Choose an overload, assuming 'source' is trying to access it. 'source' is used to perform
		// a visibility check on the potential candidates. If 'source' is null, all objects are
		// considered.
		virtual MAYBE(Named *) STORM_FN choose(NameOverloads *from, MAYBE(NameLookup *) source) const;

		// Compute the badness of a candidate. Returns -1 on no match.
		virtual Int STORM_FN matches(Named *candidate) const;

		// To string.
		virtual void STORM_FN toS(StrBuf *to) const;

		// Equals.
		virtual Bool STORM_FN equals(Object *o) const;

		// Hash.
		virtual Nat STORM_FN hash() const;
	};

	/**
	 * A NamePart which has unresolved (recursive) parameters.
	 * TODO: Support reference parameters.
	 */
	class RecPart : public NamePart {
		STORM_CLASS;
	public:
		// Create with just a name.
		STORM_CTOR RecPart(Str *name);
		STORM_CTOR RecPart(syntax::SStr *name);

		// Create with parameters as well.
		STORM_CTOR RecPart(Str *name, Array<Name *> *params);

		// Deep copy.
		virtual void STORM_FN deepCopy(CloneEnv *env);

		// Parameters.
		Array<Name *> *params;

		// Add a parameter.
		inline void STORM_FN add(Name *param) { params->push(param); }

		// Resolve.
		virtual MAYBE(SimplePart *) find(const Scope &scope);

		// To string.
		virtual void STORM_FN toS(StrBuf *to) const;
	};

}
