#pragma once
#include "Core/Array.h"
#include "Thread.h"
#include "NamedFlags.h"
#include "Value.h"
#include "Visibility.h"
#include "Scope.h"
#include "Doc.h"

namespace storm {
	STORM_PKG(core.lang);

	class Named;
	class NameSet;
	class SimpleName;
	class SimplePart;

	/**
	 * Interface for objects that can look up names.
	 */
	class NameLookup : public ObjectOn<Compiler> {
		STORM_CLASS;
	public:
		STORM_CTOR NameLookup();
		STORM_CTOR NameLookup(NameLookup *parent);

		// Find the specified NamePart in here. Returns null if not found. 'source' indicates who is
		// looking for something, and is used to perform visibility checks. If set to 'null', no
		// visibility checks are performed.
		virtual MAYBE(Named *) STORM_FN find(SimplePart *part, Scope source);

		// Convenience overloads for 'find'.
		MAYBE(Named *) STORM_FN find(Str *name, Array<Value> *params, Scope source);
		MAYBE(Named *) STORM_FN find(Str *name, Value param, Scope source);
		MAYBE(Named *) STORM_FN find(Str *name, Scope source);
		MAYBE(Named *) find(const wchar *name, Array<Value> *params, Scope source);
		MAYBE(Named *) find(const wchar *name, Value param, Scope source);
		MAYBE(Named *) find(const wchar *name, Scope source);

		// Get the parent object to this lookup, or null if none.
		virtual MAYBE(NameLookup *) STORM_FN parent() const;

		// Parent name lookup. This should be set by the parent. If it is null, the default 'parent'
		// implementation asserts. Therefore, root objects need to override 'parent' in order to
		// return null.
		MAYBE(NameLookup *) parentLookup;
	};


	/**
	 * Denotes a named object in the compiler. Named objects are for example functions, types.
	 */
	class Named : public NameLookup {
		STORM_CLASS;
	public:
		// Create without parameters.
		STORM_CTOR Named(Str *name);

		// Create with parameters.
		STORM_CTOR Named(Str *name, Array<Value> *params);

		// Our name. Note: may be null for a while during compiler startup.
		Str *name;

		// Our parameters. Note: may be null for a while during compiler startup.
		Array<Value> *params;

		// Visibility. 'null' means 'visible from everywhere'.
		MAYBE(Visibility *) visibility;

		// Documentation (if present).
		MAYBE(NamedDoc *) documentation;

		// Get a 'Doc' object for this entity. Uses 'documentation' if available, otherwise
		// generates a dummy object. This could be a fairly expensive operation, since all
		// documentation is generally *not* held in main memory.
		Doc *STORM_FN findDoc();

		// Check if this named entity is visible from 'source'.
		Bool STORM_FN visibleFrom(Scope source);
		Bool STORM_FN visibleFrom(MAYBE(NameLookup *) source);

		// Flags for this named object.
		NamedFlags flags;

		// Late initialization. Called when the type-system is up enough to initialize templates. Otherwise not needed.
		virtual void lateInit();

		// Get a path to this Named.
		SimpleName *STORM_FN path() const;

		// Get an unique human-readable identifier for this named object.
		Str *STORM_FN identifier() const;

		// Get a short version of the identifier. Only the name at this level with parameters.
		Str *STORM_FN shortIdentifier() const;

		// Better asserts for 'parent'.
		virtual NameLookup *STORM_FN parent() const;

		// Receive notifications from NameSet objects. (TODO: Move into separate class?)
		virtual void STORM_FN notifyAdded(NameSet *to, Named *added);

		// Force compilation of this named (and any sub-objects contained in here).
		virtual void STORM_FN compile();

		// String representation.
		virtual void STORM_FN toS(StrBuf *buf) const;

		// Output the visibility to a string buffer.
		void STORM_FN putVisibility(StrBuf *to) const;

	private:
		// Find closest named parent.
		Named *closestNamed() const;
	};

}
