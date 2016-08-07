#include "stdafx.h"
#include "CppLoader.h"
#include "Type.h"
#include "Engine.h"
#include "Core/Str.h"

namespace storm {

	CppLoader::CppLoader(Engine &e, const CppWorld *world) : e(e), world(world) {}

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

	void CppLoader::loadTypes(RootArray<Type> &into) {
		nat c = typeCount();
		into.resize(c);

		// Note: we do not set any names yet, as the Str type is not neccessarily available until
		// after we've created the types here.
		for (nat i = 0; i < c; i++) {
			CppType &type = world->types[i];

			// The array could be partially filled.
			if (into[i] == null) {
				into[i] = new (e) Type(null, type.flags, Size(type.size), createGcType(&type));
			}
		}

		// Now we can fill in the names properly!
		for (nat i = 0; i < c; i++) {
			CppType &type = world->types[i];

			// Just to make sure...
			if (!into[i])
				break;

			into[i]->name = new (e) Str(type.name);
		}
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

	void CppLoader::loadTemplates(RootArray<TemplateList> &into) {
		nat c = templateCount();
		into.resize(c);

		for (nat i = 0; i < c; i++) {
			CppTemplate &t = world->templates[i];

			if (!into[i])
				into[i] = new (e) TemplateList(new (e) Str(t.name), t.generate);
		}
	}

}
