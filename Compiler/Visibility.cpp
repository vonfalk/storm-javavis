#include "stdafx.h"
#include "Visibility.h"
#include "Named.h"
#include "Type.h"
#include "Package.h"
#include "Engine.h"

namespace storm {

	/**
	 * Utilities.
	 */

	// Find the first parent of 'at' that is of type T. This includes 'at' itself.
	template <class T>
	static MAYBE(T *) firstOf(NameLookup *at) {
		T *result = as<T>(at);
		while (at && !result) {
			at = at->parent();
			result = as<T>(at);
		}
		return result;
	}

	// Fidn the first parent of 'at' that is of type T, excluding 'at'.
	template <class T>
	static MAYBE(T *) firstParent(NameLookup *at) {
		if (at)
			return firstOf<T>(at->parent());
		else
			return null;
	}

	// See if 'check' is a child of 'parent'. Always returns 'null' if 'parent' is 'null'.
	static Bool hasParent(NameLookup *check, MAYBE(NameLookup *) parent) {
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

	void Public::toS(StrBuf *to) const {
		*to << S("public");
	}


	/**
	 * Private within types.
	 */

	TypePrivate::TypePrivate() {}

	Bool TypePrivate::visible(Named *check, NameLookup *source) {
		Type *type = firstParent<Type>(check);
		return hasParent(source, type);
	}

	void TypePrivate::toS(StrBuf *to) const {
		*to << S("private");
	}


	/**
	 * Protected.
	 */

	TypeProtected::TypeProtected() {}

	Bool TypeProtected::visible(Named *check, NameLookup *source) {
		Type *type = firstParent<Type>(check);
		Type *src = firstOf<Type>(source);

		if (!type || !src)
			return false;

		return src->isA(type);
	}

	void TypeProtected::toS(StrBuf *to) const {
		*to << S("protected");
	}


	/**
	 * Private within a package.
	 */

	PackagePrivate::PackagePrivate() {}

	Bool PackagePrivate::visible(Named *check, NameLookup *source) {
		Package *package = firstParent<Package>(check);
		return hasParent(source, package);
	}

	void PackagePrivate::toS(StrBuf *to) const {
		*to << S("package private");
	}


	/**
	 * Object access.
	 */

	Visibility *STORM_NAME(allPublic, public)(EnginePtr e) {
		return e.v.visibility(Engine::vPublic);
	}

	Visibility *typePrivate(EnginePtr e) {
		return e.v.visibility(Engine::vTypePrivate);
	}

	Visibility *typeProtected(EnginePtr e) {
		return e.v.visibility(Engine::vTypeProtected);
	}

	Visibility *packagePrivate(EnginePtr e) {
		return e.v.visibility(Engine::vPackagePrivate);
	}

}
