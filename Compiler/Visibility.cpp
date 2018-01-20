#include "stdafx.h"
#include "Visibility.h"
#include "Named.h"
#include "Type.h"
#include "Package.h"

namespace storm {

	/**
	 * Utilities.
	 */

	// Find the first parent of the given type.
	template <class T>
	static T *firstParent(NameLookup *at) {
		T *result = as<T>(at);
		while (at && !result) {
			at = at->parent();
			result = as<T>(at);
		}
		return result;
	}

	// See if 'check' is a child of 'parent'.
	static Bool hasParent(NameLookup *check, NameLookup *parent) {
		for (NameLookup *at = check; at; at = at->parent())
			if (at == parent)
				return true;
		return false;
	}


	/**
	 * Base class.
	 */

	Visibility::Visibility() {}

	Bool Visibility::visible(Named *check, NameLookup *source) {
		assert(false, L"Override Visibility::visible!");
		return false;
	}


	/**
	 * Public.
	 */

	Public::Public() {}

	Bool Public::visible(Named *check, NameLookup *source) {
		return true;
	}


	/**
	 * Private within types.
	 */

	TypePrivate::TypePrivate() {}

	Bool TypePrivate::visible(Named *check, NameLookup *source) {
		Type *type = firstParent<Type>(check);
		return hasParent(source, type);
	}


	/**
	 * Protected.
	 */

	TypeProtected::TypeProtected() {}

	Bool TypeProtected::visible(Named *check, NameLookup *source) {
		Type *type = firstParent<Type>(check);
		Type *src = firstParent<Type>(source);

		if (!type || !src)
			return false;

		return src->isA(type);
	}


	/**
	 * Private within a package.
	 */

	PackagePrivate::PackagePrivate() {}

	Bool PackagePrivate::visible(Named *check, NameLookup *source) {
		Package *package = firstParent<Package>(check);
		return hasParent(source, package);
	}

}
