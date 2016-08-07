#pragma once
#include "Core/Array.h"
#include "Value.h"

namespace storm {
	STORM_PKG(core.lang);

	class SimplePart;
	class Scope;
	class Name;

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

		// Copy.
		STORM_CTOR NamePart(NamePart *o);

		// Deep copy.
		virtual void STORM_FN deepCopy(CloneEnv *env);

		// Our name.
		Str *name;

		// Resolve names. NOTE: Not exposed to Storm as it has to be executed on the compiler thread.
		virtual MAYBE(SimplePart *) find(const Scope &scope);

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

		// Create with name and parameters.
		STORM_CTOR SimplePart(Str *name, Array<Value> *params);

		// Deep copy.
		virtual void STORM_FN deepCopy(CloneEnv *env);

		// Parameters.
		Array<Value> *params;

		// Resolve names.
		virtual MAYBE(SimplePart *) find(const Scope &scope);

		// To string.
		virtual void STORM_FN toS(StrBuf *to) const;
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

		// Create with parameters as well.
		STORM_CTOR RecPart(Str *name, Array<Name *> *params);

		// Deep copy.
		virtual void STORM_FN deepCopy(CloneEnv *env);

		// Parameters.
		Array<Name *> *params;

		// Resolve.
		virtual MAYBE(SimplePart *) find(const Scope &scope);

		// To string.
		virtual void STORM_FN toS(StrBuf *to) const;
	};

}
