#pragma once
#include "Core/TObject.h"
#include "Core/EnginePtr.h"

namespace storm {
	STORM_PKG(core.lang);

	class Named;
	class NameLookup;

	/**
	 * Class describing what parts of the system are able to access a specific named entity
	 * (ie. what parts of the system are visible to other parts). Instances of this class are not
	 * associated with specific Named entities, which means that it is possible to use one instance
	 * for multiple Named objects.
	 *
	 * This is an abstract class which is overridden with some default behaviours below, such as
	 * 'public', 'private', etc.
	 *
	 * Note: Virtual dispatch is resolved by examining if the overridden function is visible from
	 * the overriding function, not by examining all possible implementations from the *call
	 * site*. Furthermore, the system assumes that visibility for functions inside types are
	 * transitive wrt inheritance. Ie. if A <- B <- C, then if A.foo is visible from C.foo, then
	 * A.foo must also be visible from B.foo, and B.foo must be visible from C.foo. If this
	 * constraint is not fullfilled, then the use of virtual dispatch will be unpredictable.
	 */
	class Visibility : public ObjectOn<Compiler> {
		STORM_CLASS;
	public:
		// Create.
		Visibility();

		// Is 'check' visible to 'source'? Note that 'source' is not required to have a name, but it
		// needs to reside somewhere in the name tree.
		virtual Bool STORM_FN visible(Named *check, NameLookup *source);
	};


	/**
	 * Public visibility. Accessible to everyone.
	 */
	class Public : public Visibility {
		STORM_CLASS;
	public:
		// Create.
		Public();

		// Check.
		virtual Bool STORM_FN visible(Named *check, NameLookup *source);

	protected:
		// To string.
		virtual void STORM_FN toS(StrBuf *to) const;
	};


	/**
	 * Private visibility within a type. Accessible only to members of the same type.
	 */
	class TypePrivate : public Visibility {
		STORM_CLASS;
	public:
		// Create.
		TypePrivate();

		// Check.
		virtual Bool STORM_FN visible(Named *check, NameLookup *source);

	protected:
		// To string.
		virtual void STORM_FN toS(StrBuf *to) const;
	};


	/**
	 * Protected within a type. Accessible to members of the same type, and to members of a derived type.
	 */
	class TypeProtected : public Visibility {
		STORM_CLASS;
	public:
		// Create.
		TypeProtected();

		// Check.
		virtual Bool STORM_FN visible(Named *check, NameLookup *source);

	protected:
		// To string.
		virtual void STORM_FN toS(StrBuf *to) const;
	};


	/**
	 * Private within a package. Only the current package, or sub-packages, are able to access the type.
	 */
	class PackagePrivate : public Visibility {
		STORM_CLASS;
	public:
		// Create.
		PackagePrivate();

		// Check.
		virtual Bool STORM_FN visible(Named *check, NameLookup *source);

	protected:
		// To string.
		virtual void STORM_FN toS(StrBuf *to) const;
	};


	/**
	 * Access the shared instances of the above objects.
	 */

	Visibility *STORM_FN STORM_NAME(allPublic, public)(EnginePtr e) ON(Compiler);
	Visibility *STORM_FN typePrivate(EnginePtr e) ON(Compiler);
	Visibility *STORM_FN typeProtected(EnginePtr e) ON(Compiler);
	Visibility *STORM_FN packagePrivate(EnginePtr e) ON(Compiler);

}
