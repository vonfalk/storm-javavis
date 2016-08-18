#include "stdafx.h"
#include "CppLoader.h"
#include "Type.h"
#include "Engine.h"
#include "Package.h"
#include "Core/Str.h"

namespace storm {

	CppLoader::CppLoader(Engine &e, const CppWorld *world, World &into) :
		e(e), world(world), into(into), threads(e.gc) {}

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

	nat CppLoader::threadCount() const {
		nat n;
		for (n = 0; world->threads[n].name; n++)
			;
		return n;
	}

	void CppLoader::loadTypes() {
		nat c = typeCount();
		into.types.resize(c);

		// Note: we do not set any names yet, as the Str type is not neccessarily available until
		// after we've created the types here.
		for (nat i = 0; i < c; i++) {
			CppType &type = world->types[i];

			// The array could be partially filled.
			if (into.types[i] == null && type.kind != CppType::superCustom) {
				into.types[i] = new (e) Type(null, type.flags, Size(type.size), createGcType(&type));
			}
		}

		// Create all types with custom types.
		for (nat i = 0; i < c; i++) {
			CppType &type = world->types[i];

			if (type.kind != CppType::superCustom)
				continue;
			if (into.types[i])
				continue;

			CppType::CreateFn fn = (CppType::CreateFn)type.super;
			into.types[i] = (*fn)(new (e) Str(type.name), Size(type.size), createGcType(&type));
		}

		// Now we can fill in the names and superclasses properly!
		for (nat i = 0; i < c; i++) {
			CppType &type = world->types[i];

			// Just to make sure...
			if (!into.types[i])
				break;

			into.types[i]->name = new (e) Str(type.name);
		}
	}

	void CppLoader::loadThreads() {
		nat c = threadCount();
		into.threads.resize(c);
		threads.resize(c);

		for (nat i = 0; i < c; i++) {
			CppThread &thread = world->threads[i];

			if (into.threads[i]) {
				into.threads[i] = new (e) Thread(thread.decl->createFn);
			}

			threads[i] = new (e) NamedThread(new (e) Str(thread.name), into.threads[i]);
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

					into.types[i]->setSuper(into.types[type.super]);
					break;
				case CppType::superThread:
					into.types[i]->setThread(threads[type.super]);
					break;
				case CppType::superCustom:
					// Already done.
					break;
				default:
					assert(false, L"Unknown kind on type " + ::toS(type.name) + L": " + ::toS(type.kind) + L".");
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
		into.templates.resize(c);

		for (nat i = 0; i < c; i++) {
			CppTemplate &t = world->templates[i];

			if (!into.templates[i]) {
				Str *n = new (e) Str(t.name);
				TemplateFn *templ = new (e) TemplateFn(n, t.generate);
				into.templates[i] = new (e) TemplateList(templ);
			}
		}
	}

	NameSet *CppLoader::findPkg(const wchar *name) {
		SimpleName *pkgName = parseSimpleName(e, name);
		NameSet *r = e.nameSet(pkgName);
		assert(r, L"Failed to find the package " + String(name));
		return r;
	}

	void CppLoader::loadPackages() {
		nat c = typeCount();
		for (nat i = 0; i < c; i++) {
			CppType &t = world->types[i];
			findPkg(t.pkg)->add(into.types[i]);
		}

		c = templateCount();
		for (nat i = 0; i < c; i++) {
			CppTemplate &t = world->templates[i];

			NameSet *to = findPkg(t.pkg);
			into.templates[i]->addTo(to);
		}

		c = threadCount();
		for (nat i = 0; i < c; i++) {
			CppThread &t = world->threads[i];
			findPkg(t.pkg)->add(threads[i]);
		}
	}

}
