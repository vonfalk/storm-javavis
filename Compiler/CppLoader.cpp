#include "stdafx.h"
#include "CppLoader.h"
#include "Type.h"
#include "Engine.h"
#include "Package.h"
#include "Function.h"
#include "License.h"
#include "Version.h"
#include "Code.h"
#include "VTableCpp.h"
#include "Exception.h"
#include "Lib/Maybe.h"
#include "Lib/Enum.h"
#include "Core/Str.h"

namespace storm {

	CppLoader::CppLoader(Engine &e, const CppWorld *world, World &into, Package *root, Url *docPath) :
		e(&e), world(world), into(&into), rootPackage(root), docUrl(null) {

		if (docPath)
			docUrl = docPath->push(new (e) Str(world->docFile));
	}

	Nat CppLoader::typeCount() const {
		Nat n = 0;
		while (world->types[n].name)
			n++;
		return n;
	}

	Nat CppLoader::templateCount() const {
		Nat n = 0;
		while (world->templates[n].name)
			n++;
		return n;
	}

	Nat CppLoader::threadCount() const {
		Nat n = 0;
		while (world->threads[n].name)
			n++;
		return n;
	}

	Nat CppLoader::functionCount() const {
		Nat n = 0;
		while (world->functions[n].name)
			n++;
		return n;
	}

	Nat CppLoader::variableCount() const {
		Nat n = 0;
		while (world->variables[n].name)
			n++;
		return n;
	}

	Nat CppLoader::enumValueCount() const {
		Nat n = 0;
		while (world->enumValues[n].name)
			n++;
		return n;
	}

	Nat CppLoader::licenseCount() const {
		Nat n = 0;
		while (world->licenses[n].name)
			n++;
		return n;
	}

	Nat CppLoader::versionCount() const {
		Nat n = 0;
		while (world->versions[n].name)
			n++;
		return n;
	}

	static const void *typeVTable(const CppType &t) {
		if (t.vtable)
			return (*t.vtable)();
		else
			return null;
	}

	Type *CppLoader::createType(Nat id, const CppType &type) {
		TypeFlags flags = type.flags;
		// Validate the C++ flags in here.
		if ((flags & typeCppPOD) && (flags & typeValue)) {
			throw BuiltInError(L"The class " + ::toS(type.pkg) + L"." + ::toS(type.name) +
							L" is a POD type, which is not supported by Storm. "
							L"Add a constructor to the class (it does not have to be exposed to Storm) "
							L"and compile again.");
		}

		GcType *gcType = createGcType(id);

		// If this type inherits from 'Type', it needs special care in its Gc-description.
		if (type.kind == CppType::superClassType) {
			GcType *t = Type::makeType(*e, gcType);
			e->gc.freeType(gcType);
			gcType = t;
		}

		return new (*e) Type(null, flags, Size(type.size), gcType, typeVTable(type));
	}

	Type *CppLoader::findType(const CppType &type) {
		SimpleName *name = parseSimpleName(*e, type.pkg);
		name->add(new (*e) Str(type.name));
		Type *t = as<Type>(e->scope().find(name));
		if (!t)
			throw BuiltInError(L"Failed to locate " + ::toS(name));
		return t;
	}

	void CppLoader::loadTypes() {
		Nat c = typeCount();
		into->types.resize(c);

		// Note: we do not set any names yet, as the Str type is not neccessarily available until
		// after we've created the types here.
		for (Nat i = 0; i < c; i++) {
			const CppType &type = world->types[i];

			// The array could be partially filled.
			if (into->types[i] == null && !delayed(type)) {
				if (external(type))
					into->types[i] = findType(type);
				else
					into->types[i] = createType(i, type);
			}
		}

		// Create all types with custom types.
		for (Nat i = 0; i < c; i++) {
			const CppType &type = world->types[i];

			if (!delayed(type))
				continue;
			if (into->types[i])
				continue;

			CppType::CreateFn fn = (CppType::CreateFn)type.super;
			if (type.kind == CppType::superEnum)
				fn = &createEnum;
			else if (type.kind == CppType::superBitmaskEnum)
				fn = &createBitmaskEnum;
			into->types[i] = (*fn)(new (*e) Str(type.name), Size(type.size), createGcType(i));
		}

		// If we're in early boot, we need to manually create vtables for all types.
		if (!e->has(bootTypes)) {
			for (Nat i = 0; i < c; i++) {
				into->types[i]->vtableInit(typeVTable(world->types[i]));
			}
		}

		// Now we can fill in the names and visibility properly!
		for (Nat i = 0; i < c; i++) {
			const CppType &type = world->types[i];

			// Don't mess with external types.
			if (external(type))
				continue;

			// Just to make sure...
			if (!into->types[i])
				continue;

			// Name.
			into->types[i]->name = new (*e) Str(type.name);

			// Visibility. All types from C++ are public. There is no mechanism to make them anything else.
			into->types[i]->visibility = e->visibility(Engine::vPublic);

			// Documentation.
			setDoc(into->types[i], type.doc, null);
		}
	}

	void CppLoader::loadThreads() {
		Nat c = threadCount();
		into->threads.resize(c);
		into->namedThreads.resize(c);

		for (Nat i = 0; i < c; i++) {
			const CppThread &thread = world->threads[i];

			if (external(thread)) {
				SimpleName *name = parseSimpleName(*e, thread.pkg);
				name->add(new (*e) Str(thread.name));
				NamedThread *found = as<NamedThread>(e->scope().find(name));
				if (!found)
					throw BuiltInError(L"Failed to locate " + ::toS(name));

				into->namedThreads[i] = found;
				into->threads[i] = found->thread();
			} else {
				if (!into->threads[i]) {
					into->threads[i] = new (*e) Thread(thread.decl->createFn);
				}

				into->namedThreads[i] = new (*e) NamedThread(new (*e) Str(thread.name), into->threads[i]);
				into->namedThreads[i]->visibility = e->visibility(Engine::vPublic);
				setDoc(into->namedThreads[i], thread.doc, null);
			}
		}
	}

	void CppLoader::loadSuper() {
		Nat c = typeCount();

		// Remember which ones we've already set.
		vector<bool> updated(c, false);

		do {
			// Try to add super classes to all classes until we're done. We're only adding classes
			// as a super class when they have their super classes added, to avoid re-computation of
			// lookup tables and similar.
			for (Nat i = 0; i < c; i++) {
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

					into->types[i]->setSuper(into->types[type.super]);
					break;
				case CppType::superThread:
					if (TObject::stormType(*e) == null)
						continue;

					into->types[i]->setThread(into->namedThreads[type.super]);
					break;
				case CppType::superCustom:
				case CppType::superEnum:
				case CppType::superBitmaskEnum:
					// Already done.
					break;
				case CppType::superExternal:
					// Nothing to do!
					break;
				default:
					assert(false, L"Unknown kind on type " + ::toS(type.name) + L": " + ::toS(type.kind) + L".");
					break;
				}

				updated[i] = true;
			}
		} while (!all(updated));
	}

	GcType *CppLoader::createGcType(Nat id) {
		const CppType &type = world->types[id];

		Nat entries;
		for (entries = 0; type.ptrOffsets[entries] != CppOffset::invalid; entries++)
			;

		GcType *t = e->gc.allocType(GcType::tFixedObj, null, Size(type.size).current(), entries);

		for (Nat i = 0; i < entries; i++) {
			t->offset[i] = Offset(type.ptrOffsets[i]).current();
		}

		t->finalizer = type.destructor;

#ifdef DEBUG
		if (!world->refTypes) {
			// Don't crash if someone missed their data.
			WARNING(L"Missing debug offset data when loading C++ functions!");
		} else {
			const CppRefType &ref = world->refTypes[id];

			// Note: t->stride == 0 means that sizeof(Type) will be 1.
			// Note: ref.size == 0 means that this type does not exist in C++.
			assert(t->stride == 0 || ref.size == 0 || t->stride == ref.size, L"Type size mismatch for " +
				::toS(type.name) + L". Storm size: " + ::toS(t->stride) + L", C++ size: " + ::toS(ref.size) + L".");

			for (Nat i = 0; i < entries; i++) {
				// Uncheckable?
				if (ref.offsets[i] == size_t(-1))
					continue;

				assert(t->offset[i] == ref.offsets[i], L"Type offset mismatch for " + ::toS(type.name) + L" member #" +
					::toS(i) + L". Storm offset: " + ::toS(t->offset[i]) + L", C++ offset: " + ::toS(ref.offsets[i]));
			}

			assert(ref.offsets[entries] == size_t(-1), L"There is more data for " + ::toS(type.name));
		}
#endif

		return t;
	}

	void CppLoader::loadTemplates() {
		Nat c = templateCount();
		into->templates.resize(c);

		for (Nat i = 0; i < c; i++) {
			const CppTemplate &t = world->templates[i];

			if (!into->templates[i]) {
				into->templates[i] = loadTemplate(t);
				// TODO: Documentation for templates!
			}
		}
	}

	TemplateList *CppLoader::loadTemplate(const CppTemplate &t) {
		Str *n = new (*e) Str(t.name);
		TemplateCppFn *templ = null;
		if (external(t))
			templ = new (*e) TemplatePlaceholder(n);
		else
			templ = new (*e) TemplateCppFn(n, t.generate);

		TemplateList *result = new (*e) TemplateList(into, templ);

		if (external(t)) {
			// Attach the template to the correct package right now, otherwise it can not be used.
			NameSet *pkg = findAbsPkg(t.pkg);
			if (!pkg)
				throw InternalError(L"Could not find the package " + ::toS(t.pkg) + L"!");
			result->addTo(pkg);
		}

		return result;
	}

	NameSet *CppLoader::findPkg(const wchar *name) {
		if (!rootPackage)
			rootPackage = e->package();

		SimpleName *pkgName = parseSimpleName(*e, name);
		NameSet *r = e->nameSet(pkgName, rootPackage, true);
		assert(r, L"Failed to find the package or type " + String(name));
		return r;
	}

	NameSet *CppLoader::findAbsPkg(const wchar *name) {
		SimpleName *pkgName = parseSimpleName(*e, name);
		NameSet *r = e->nameSet(pkgName, e->package(), false);
		assert(r, L"Failed to find the package or type " + String(name));
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
				assert(count < maxElem, L"Too many template parameters used in C++!");
				if (ref.params[count] == CppTypeRef::tVoid)
					elems[count] = -1;
				else
					elems[count] = Nat(ref.params[count]);
			}

			result = Value(into->templates[ref.id]->find(elems, count));
		} else if (ref.id != CppTypeRef::invalid) {
			// Regular type.
			result = Value(into->types[ref.id]);
		} else {
			// Void.
			return Value();
		}

		if (ref.ref) {
			// Not applicable to class types.
			if (!result.isHeapObj())
				result = result.asRef(true);
		}

		if (ref.maybe) {
			result = wrapMaybe(result);
		}

		return result;
	}

	void CppLoader::loadPackages() {
		Nat c = typeCount();
		for (Nat i = 0; i < c; i++) {
			const CppType &t = world->types[i];
			if (!external(t))
				findPkg(t.pkg)->add(into->types[i]);
		}

		c = templateCount();
		for (Nat i = 0; i < c; i++) {
			const CppTemplate &t = world->templates[i];
			if (!external(t)) {
				NameSet *to = findPkg(t.pkg);
				into->templates[i]->addTo(to);
			}
		}

		c = threadCount();
		for (Nat i = 0; i < c; i++) {
			const CppThread &t = world->threads[i];
			if (!external(t))
				findPkg(t.pkg)->add(into->namedThreads[i]);
		}
	}


	void CppLoader::loadFunctions() {
		// This is fairly expensive and generates a bit of garbage. Help the GC by informing it of
		// the mess we're going to create.
		Gc::RampAlloc z(e->gc);

		Nat c = functionCount();
		for (Nat i = 0; i < c; i++) {
			loadFunction(world->functions[i]);
		}
	}

	void CppLoader::loadFunction(const CppFunction &fn) {
		switch (fnKind(fn)) {
		case CppFunction::fnFree:
		case CppFunction::fnFreeEngine:
		case CppFunction::fnStatic:
			loadFreeFunction(fn);
			break;
		case CppFunction::fnMember:
			loadMemberFunction(fn, false);
			break;
		case CppFunction::fnCastMember:
			loadMemberFunction(fn, true);
			break;
		default:
			assert(false, L"Unknown function kind: " + ::toS(fn.kind));
			break;
		}
	}

	void CppLoader::loadFreeFunction(const CppFunction &fn) {
		NameSet *into = findPkg(fn.pkg);

		Value result = findValue(fn.result);

		Function *f = new (*e) Function(result, new (*e) Str(fn.name), loadFnParams(fn.params));

		if (fnKind(fn) == CppFunction::fnFreeEngine)
			f->setCode(new (*e) StaticEngineCode(fn.ptr));
		else
			f->setCode(new (*e) StaticCode(fn.ptr));

		if (fn.threadId < this->into->namedThreads.count())
			f->runOn(this->into->namedThreads[fn.threadId]);

		if (fnHasFlag(fn, CppFunction::fnAssign))
			f->make(fnAssign);

		if (fnKind(fn) == CppFunction::fnStatic)
			f->make(fnStatic);

		// Note: CppFunction::fnFinal has no meaning for free functions, so we can ignore it here.

		f->visibility = visibility(fn.access);
		setDoc(f, fn.doc, fn.params);

		into->add(f);
	}

	void CppLoader::loadMemberFunction(const CppFunction &fn, bool cast) {
		Value result = findValue(fn.result);
		Array<Value> *params = loadFnParams(fn.params);
		assert(params->count() > 0, L"Missing this pointer for " + ::toS(fn.name) + L"!");

		// Find the vtable...
		const CppParam &firstParam = fn.params[0];
		assert(firstParam.type.params == null, L"Members for template types are not supported!");
		// CppType::VTableFn vFn = world->types[firstParam.id].vtable;

		const void *ptr = deVirtualize(firstParam.type, fn.ptr);
		Function *f = new (*e) CppMemberFunction(result, new (*e) Str(fn.name), params, fn.ptr);
		f->setCode(new (*e) StaticCode(ptr));

		if (cast)
			f->makeAutoCast();

		if (fn.threadId < into->namedThreads.count())
			f->runOn(into->namedThreads[fn.threadId]);

		if (wcscmp(Type::CTOR, fn.name) == 0 || wcscmp(Type::DTOR, fn.name) == 0) {
			// Check if the type was declared as a simple type.
			Type *owner = params->at(0).type;
			if (owner->typeFlags & typeCppSimple)
				// If so, it has a pure constructor.
				f->makePure();
		}

		if (fnHasFlag(fn, CppFunction::fnAssign))
			f->make(fnAssign);

		if (fnHasFlag(fn, CppFunction::fnFinal))
			f->make(fnFinal);

		f->visibility = visibility(fn.access);
		setDoc(f, fn.doc, fn.params);

		params->at(0).type->add(f);
	}

	Array<Value> *CppLoader::loadFnParams(const CppParam *params) {
		Array<Value> *r = new (*e) Array<Value>();

		for (Value v = findValue(params->type); v != Value(); v = findValue((++params)->type)) {
			r->push(v);
		}

		return r;
	}

	const void *CppLoader::findVTable(const CppTypeRef &ref) {
		assert(ref.params == null, L"Members of template types are not supported!");
		CppType::VTableFn f = world->types[ref.id].vtable;
		if (f)
			return (*f)();
		else
			return null;
	}

	const void *CppLoader::deVirtualize(const CppTypeRef &ref, const void *fn) {
		const void *tab = findVTable(ref);
		if (!tab)
			return fn;
		const void *f = vtable::deVirtualize(tab, fn);
		if (!f)
			return fn;
		return f;
	}

	void CppLoader::loadVariables() {
		Nat c = variableCount();
		for (Nat i = 0; i < c; i++) {
			loadVariable(world->variables[i]);
		}

		c = enumValueCount();
		for (Nat i = 0; i < c; i++) {
			loadEnumValue(world->enumValues[i]);
		}
	}

	void CppLoader::loadVariable(const CppVariable &var) {
		Type *memberOf = into->types[var.memberOf];
		assert(memberOf, L"Type not properly loaded!");

		Value type = findValue(var.type);
		assert(type != Value(), L"Type of the variable is void!");

		MemberVar *v = new (*e) MemberVar(new (*e) Str(var.name), type, memberOf);
		v->setOffset(Offset(var.offset));
		v->visibility = visibility(var.access);
		setDoc(v, var.doc, null);
		memberOf->add(v);
	}

	void CppLoader::loadEnumValue(const CppEnumValue &val) {
		Enum *memberOf = as<Enum>(into->types[val.memberOf]);
		assert(memberOf, L"Type not properly loaded or not an enum!");

		EnumValue *value = new (*e) EnumValue(memberOf, new (*e) Str(val.name), val.value);
		setDoc(value, val.doc, null);
		memberOf->add(value);
	}

	void CppLoader::loadMeta() {
		loadLicenses();
		loadVersions();
	}

	void CppLoader::loadLicenses() {
		Nat count = licenseCount();
		for (Nat i = 0; i < count; i++) {
			const CppLicense &l = world->licenses[i];
			NameSet *into = findPkg(l.pkg);
			into->add(new (*e) License(new (*e) Str(l.name), new (*e) Str(l.title), new (*e) Str(l.body)));
		}
	}

	void CppLoader::loadVersions() {
		Nat count = versionCount();
		for (Nat i = 0; i < count; i++) {
			const CppVersion &v = world->versions[i];
			Version *ver = parseVersion(new (*e) Str(v.version));
			if (!ver)
				throw InternalError(L"Failed to parse the version string " + ::toS(v.version));

			NameSet *into = findPkg(v.pkg);
			into->add(new (*e) VersionTag(new (*e) Str(v.name), ver));
		}
	}

	Visibility *CppLoader::visibility(CppAccess a) {
		switch (a) {
		case cppPublic:
			return e->visibility(Engine::vPublic);
		case cppProtected:
			return e->visibility(Engine::vTypeProtected);
		case cppPrivate:
			return e->visibility(Engine::vTypePrivate);
		default:
			assert(false, L"Unknown access in C++ code: " + ::toS(a));
			return null;
		}
	}

	CppDoc *CppLoader::createDoc(Named *entity, Nat id, const CppParam *params) {
		if (id <= 0)
			return null;

		if (docUrl)
			return new (*e) CppDoc(entity, docUrl, id, params);
		else
			return new (*e) CppDoc(entity, world->docFile, id, params);
	}

	void CppLoader::setDoc(Named *entity, Nat id, const CppParam *params) {
		entity->documentation = createDoc(entity, id, params);
	}

}
