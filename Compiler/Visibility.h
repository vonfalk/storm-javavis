#pragma once
#include "Core/TObject.h"

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
	};

}
