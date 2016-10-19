#include "stdafx.h"
#include "CppLoader.h"
#include "Type.h"
#include "Engine.h"
#include "Package.h"
#include "Function.h"
#include "Code.h"
#include "Lib/MaybeTemplate.h"
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

	nat CppLoader::functionCount() const {
		nat n;
		for (n = 0; world->functions[n].name; n++)
			;
		return n;
	}

	void CppLoader::loadTypes() {
		nat c = typeCount();
		into.types.resize(c);

		// Note: we do not set any names yet, as the Str type is not neccessarily available until
		// after we've created the types here.
		for (nat i = 0; i < c; i++) {
			const CppType &type = world->types[i];

			// The array could be partially filled.
			if (into.types[i] == null && type.kind != CppType::superCustom) {
				GcType *gcType = createGcType(&type, i);

				// If this type inherits from 'Type', it needs special care in its Gc-description.
				if (type.kind == CppType::superClassType) {
					GcType *t = Type::makeType(e, gcType);
					e.gc.freeType(gcType);
					gcType = t;
				}

				into.types[i] = new (e) Type(null, type.flags, Size(type.size), gcType);
			}
		}

		// Create all types with custom types.
		for (nat i = 0; i < c; i++) {
			const CppType &type = world->types[i];

			if (type.kind != CppType::superCustom)
				continue;
			if (into.types[i])
				continue;

			CppType::CreateFn fn = (CppType::CreateFn)type.super;
			into.types[i] = (*fn)(new (e) Str(type.name), Size(type.size), createGcType(&type, i));
		}

		// Now we can fill in the names and superclasses properly!
		for (nat i = 0; i < c; i++) {
			const CppType &type = world->types[i];

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
			const CppThread &thread = world->threads[i];

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
				const CppType &type = world->types[i];
				if (updated[i])
					continue;

				switch (type.kind) {
				case CppType::superNone:
					// Nothing to do.
					break;
				case CppType::superClass:
				case CppType::superClassType:
					// Delay update?
					if (!updated[type.super])
						continue;

					into.types[i]->setSuper(into.types[type.super]);
					break;
				case CppType::superThread:
					if (TObject::stormType(e) == null)
						continue;

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

	GcType *CppLoader::createGcType(const CppType *type, Nat id) {
		nat entries;
		for (entries = 0; type->ptrOffsets[entries] != CppOffset::invalid; entries++)
			;

		GcType *t = e.gc.allocType(GcType::tFixed, null, Size(type->size).current(), entries);

		for (nat i = 0; i < entries; i++) {
			t->offset[i] = Offset(type->ptrOffsets[i]).current();
		}

		t->finalizer = type->destructor;

#ifdef DEBUG
		// TODO: Clean up this when we're loading external dlls!
		const size_t *refData = cppRefOffsets()[id];
		// sizeof() never returns zero, but we support zero-sized types properly.
		if (t->stride > 0) {
			assert(t->stride == refData[0], L"Type size mismatch for " + ::toS(type->name)
				+ L": " + ::toS(t->stride) + L" vs " + ::toS(refData[0]));
		}
		refData = refData + 1;

		for (nat i = 0; i < entries; i++) {
			// Uncheckable?
			if (refData[i] == size_t(-2))
				continue;

			assert(t->offset[i] == refData[i], L"Type size mismatch for " + ::toS(type->name) + L" at " +
				::toS(i) + L": " + ::toS(t->offset[i]) + L" vs " + ::toS(refData[i]));
		}
		assert(refData[entries] == size_t(-1), L"There is more data for " + ::toS(type->name));
#endif

		return t;
	}

	void CppLoader::loadTemplates() {
		nat c = templateCount();
		into.templates.resize(c);

		for (nat i = 0; i < c; i++) {
			const CppTemplate &t = world->templates[i];

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

	Value CppLoader::findValue(const CppTypeRef &ref) {
		Value result;
		if (ref.params) {
			// Template!
			const Nat maxElem = 20;
			Nat elems[maxElem];
			Nat count = 0;
			for (count = 0; count < maxElem && ref.params[count] != CppTypeRef::invalid; count++) {
				if (ref.params[count] == CppTypeRef::tVoid)
					elems[count] = -1;
				else
					elems[count] = Nat(ref.params[count]);
			}

			result = Value(into.templates[ref.id]->find(elems, count));
		} else if (ref.id != CppTypeRef::invalid) {
			// Regular type.
			result = Value(into.types[ref.id]);
		} else {
			// Void.
			return Value();
		}

		if (ref.maybe) {
			result = wrapMaybe(result);
		}

		return result;
	}

	void CppLoader::loadPackages() {
		nat c = typeCount();
		for (nat i = 0; i < c; i++) {
			const CppType &t = world->types[i];
			findPkg(t.pkg)->add(into.types[i]);
		}

		c = templateCount();
		for (nat i = 0; i < c; i++) {
			const CppTemplate &t = world->templates[i];

			NameSet *to = findPkg(t.pkg);
			into.templates[i]->addTo(to);
		}

		c = threadCount();
		for (nat i = 0; i < c; i++) {
			const CppThread &t = world->threads[i];
			findPkg(t.pkg)->add(threads[i]);
		}
	}


	void CppLoader::loadFunctions() {
		nat c = functionCount();
		for (nat i = 0; i < c; i++) {
			loadFunction(world->functions[i]);
		}
	}

	void CppLoader::loadFunction(const CppFunction &fn) {
		switch (fn.kind) {
		case CppFunction::fnFree:
		case CppFunction::fnFreeEngine:
			loadFreeFunction(fn);
			break;
		case CppFunction::fnMember:
			loadMemberFunction(fn);
			break;
		default:
			assert(false, L"Unknown function kind: " + ::toS(fn.kind));
			break;
		}
	}

	void CppLoader::loadFreeFunction(const CppFunction &fn) {
		NameSet *into = findPkg(fn.pkg);

		Value result = findValue(fn.result);

		Function *f = new (e) Function(result, new (e) Str(fn.name), loadFnParams(fn.params));

		if (fn.kind == CppFunction::fnFreeEngine)
			f->setCode(new (e) StaticEngineCode(result, fn.ptr));
		else
			f->setCode(new (e) StaticCode(fn.ptr));

		into->add(f);
	}

	void CppLoader::loadMemberFunction(const CppFunction &fn) {
		Value result = findValue(fn.result);
		Array<Value> *params = loadFnParams(fn.params);
		assert(params->count() > 0, L"Missing this pointer for " + ::toS(fn.name) + L"!");

		Function *f = new (e) Function(result, new (e) Str(fn.name), params);
		f->setCode(new (e) StaticCode(fn.ptr));

		params->at(0).type->add(f);
	}

	Array<Value> *CppLoader::loadFnParams(const CppTypeRef *params) {
		Array<Value> *r = new (e) Array<Value>();

		for (Value v = findValue(*params); v != Value(); v = findValue(*++params)) {
			r->push(v);
		}

		return r;
	}

}
