#include "stdafx.h"
#include "CppLoader.h"
#include "Type.h"
#include "Engine.h"
#include "Package.h"
#include "Core/Str.h"

namespace storm {

	CppLoader::CppLoader(Engine &e, const CppWorld *world, RootArray<Type> &types, RootArray<TemplateList> &templ) :
		e(e), world(world), types(types), templates(templ) {}

	nat CppLoader::typeCount() const {
		nat n;
		for (n = 0; world->types[n].name; n++)
			;
		return n;
	}

	nat CppLoader::templateCount() const {
		nat n;
		for (n = 0; world->templates[n].name; n++)
			;
		return n;
	}

	void CppLoader::loadTypes() {
		nat c = typeCount();
		types.resize(c);

		// Note: we do not set any names yet, as the Str type is not neccessarily available until
		// after we've created the types here.
		for (nat i = 0; i < c; i++) {
			CppType &type = world->types[i];

			// The array could be partially filled.
			if (types[i] == null) {
				types[i] = new (e) Type(null, type.flags, Size(type.size), createGcType(&type));
			}
		}

		// Now we can fill in the names and superclasses properly!
		for (nat i = 0; i < c; i++) {
			CppType &type = world->types[i];

			// Just to make sure...
			if (!types[i])
				break;

			types[i]->name = new (e) Str(type.name);
		}
	}

	void CppLoader::loadSuper() {
		nat c = typeCount();

		// Remember which ones we've already set.
		vector<bool> updated(c, false);

		do {
			// Try to add super classes to all classes until we're done. We're only adding classes
			// as a super class when they have their super classes added, to avoid re-computation of
			// lookup tables and similar.
			for (nat i = 0; i < c; i++) {
				CppType &type = world->types[i];
				if (updated[i])
					continue;

				switch (type.kind) {
				case CppType::superNone:
					// Nothing to do.
					break;
				case CppType::superClass:
					// Delay update?
					if (!updated[type.super])
						continue;

					types[i]->setSuper(types[type.super]);
					break;
				case CppType::superThread:
					assert(false, L"Can not set threads as super class yet.");
					break;
				}

				updated[i] = true;
			}
		} while (!all(updated));
	}

	GcType *CppLoader::createGcType(const CppType *type) {
		nat entries;
		for (entries = 0; type->ptrOffsets[entries] != CppOffset::invalid; entries++)
			;

		GcType *t = e.gc.allocType(GcType::tFixed, null, Size(type->size).current(), entries);

		for (nat i = 0; i < entries; i++) {
			t->offset[i] = Offset(type->ptrOffsets[i]).current();
		}

		t->finalizer = type->destructor;

		return t;
	}

	void CppLoader::loadTemplates() {
		nat c = templateCount();
		templates.resize(c);

		for (nat i = 0; i < c; i++) {
			CppTemplate &t = world->templates[i];

			if (!templates[i]) {
				Str *n = new (e) Str(t.name);
				TemplateFn *templ = new (e) TemplateFn(n, t.generate);
				templates[i] = new (e) TemplateList(templ);
			}
		}
	}

	void CppLoader::loadPackages() {
		nat c = typeCount();
		for (nat i = 0; i < c; i++) {
			CppType &t = world->types[i];

			SimpleName *pkgName = parseSimpleName(e, t.pkg);
			NameSet *into = e.nameSet(pkgName);
			assert(into, L"Failed to find the package " + String(t.pkg));
			into->add(types[i]);
		}

		PLN(L"Loaded types!");
	}

}
