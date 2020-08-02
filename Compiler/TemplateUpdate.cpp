#include "stdafx.h"
#include "TemplateUpdate.h"
#include "Engine.h"
#include "NameSet.h"
#include "Package.h"
#include "Type.h"
#include "Core/Set.h"
#include "Core/Array.h"

namespace storm {

	static void allTypes(NameSet *root, Set<Type *> *to) {
		for (NameSet::Iter i = root->begin(), end = root->end(); i != end; ++i) {
			Named *here = i.v();
			if (Type *t = as<Type>(here)) {
				to->put(t);
				allTypes(t, to);
			} else if (NameSet *s = as<NameSet>(here)) {
				allTypes(s, to);
			}
		}
	}

	static Set<Type *> *allTypes(NameSet *root) {
		Set<Type *> *result = new (root) Set<Type *>();
		allTypes(root, result);
		return result;
	}

	static Bool hasType(Set<Type *> *toRemove, Named *named) {
		for (Nat i = 0; i < named->params->count(); i++) {
			if (toRemove->has(named->params->at(i).type))
				return true;
		}
		return false;
	}

	// Returns any new types that were removed.
	static Set<Type *> *remove(Set<Type *> *toRemove, Array<NameOverloads *> *in) {
		Set<Type *> *removed = new (in) Set<Type *>();
		for (Nat i = 0; i < in->count(); i++) {
			NameOverloads *o = in->at(i);

			// We might remove stuff, so we iterate backwards.
			for (Nat j = o->count(); j > 0; j--) {
				Named *n = o->at(j - 1);
				if (hasType(toRemove, n)) {
					// Remove it.
					o->remove(n);

					// If it was a type, we need to remove any uses of that type as well.
					if (Type *t = as<Type>(n))
						removed->put(t);
				}
			}
		}
		return removed;
	}

	void removeTemplatesFrom(NameSet *root) {
		Set<Type *> *types = allTypes(root);
		Array<NameOverloads *> *check = root->engine().package()->templateOverloads();

		// Remove types unil we have nothing more to check.
		while (types->any()) {
			types = remove(types, check);
		}
	}

}
