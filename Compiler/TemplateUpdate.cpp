#include "stdafx.h"
#include "TemplateUpdate.h"
#include "Engine.h"
#include "NameSet.h"
#include "Package.h"
#include "Type.h"
#include "Exception.h"
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

	static Named *findOld(NameOverloads *in, Named *named, ReplaceContext *context) {
		for (Nat i = 0; i < in->count(); i++) {
			Named *candidate = in->at(i);

			if (candidate == named)
				continue;
			if (candidate->params->count() != named->params->count())
				continue;

			bool same = true;
			for (Nat j = 0; j < named->params->count(); j++) {
				if (context->normalize(named->params->at(j)) != candidate->params->at(j)) {
					same = false;
					break;
				}
			}

			// Found it!
			if (same)
				return candidate;
		}

		return null;
	}

	static Set<Type *> *replace(Set<Type *> *toReplace, Array<NameOverloads *> *in,
							ReplaceContext *context, Array<NamedPair> *result) {
		Set<Type *> *replaced = new (in) Set<Type *>();
		for (Nat i = 0; i < in->count(); i++) {
			NameOverloads *o = in->at(i);

			for (Nat j = 0; j < o->count(); j++) {
				Named *n = o->at(j);

				if (!hasType(toReplace, n))
					continue;

				Named *old = findOld(o, n, context);

				// If an old version did not exist, we don't need to worry.
				if (!old)
					continue;

				result->push(NamedPair(old, n));

				// Update type equivalences if needed.
				Type *oldType = as<Type>(old);
				Type *newType = as<Type>(n);
				if ((oldType == null) != (newType == null)) {
					StrBuf *msg = new (in) StrBuf();
					*msg << S("Unable to replace a type with a non-type during template replacement.\n");
					*msg << S("Old entity: ") << old << S("\n");
					*msg << S("New entity: ") << n;
					throw new (in) ReplaceError(oldType->pos, msg->toS());
				} else if (newType) {
					context->addEq(oldType, newType);
					// If it was a type, we need to search for any dependencies of that type as well.
					replaced->put(newType);
				}
			}
		}

		return replaced;
	}

	Array<NamedPair> *replaceTemplatesFrom(NameSet *root, ReplaceContext *context) {
		Array<NamedPair> *result = new (root) Array<NamedPair>();
		// Note: We could utilize the equivalence already built inside 'context' rather than generating our own.
		Set<Type *> *types = allTypes(root);
		Array<NameOverloads *> *check = root->engine().package()->templateOverloads();

		while (types->any()) {
			types = replace(types, check, context, result);
		}

		return result;
	}

}
