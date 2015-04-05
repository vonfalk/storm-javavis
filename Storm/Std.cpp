#include "stdafx.h"
#include "Std.h"
#include "Lib/BuiltIn.h"
#include "Lib/Int.h"
#include "Lib/Bool.h"
#include "Lib/Object.h"
#include "Lib/TObject.h"
#include "Lib/ArrayTemplate.h"
#include "Lib/FutureTemplate.h"
#include "Exception.h"
#include "Function.h"
#include "Engine.h"
#include "NamedThread.h"
#include "TypeCtor.h"
#include "Code/VTable.h"

namespace storm {

	static nat builtInCount() {
		nat count = 0;
		for (const BuiltInType *t = builtInTypes(); t->name; t++)
			count++;
		return count;
	}

	static const BuiltInType *findById(nat id) {
		for (const BuiltInType *t = builtInTypes(); t->name; t++) {
			if (t->typePtrId == id)
				return t;
		}
		return null;
	}

	static Value findValue(const Scope &src, const ValueRef &val, Engine &e) {
		if (val.name == null)
			return Value();

		Auto<Name> name = parseSimpleName(e, val.name);
		Named *f = src.find(name);
		if (Type *t = as<Type>(f)) {
			Value r(t, (val.options & ValueRef::ref) != ValueRef::nothing);

			if (val.options & ValueRef::array) {
				Type *array = arrayType(e, r);
				r = Value(array);
			}

			return r;
		}

		throw BuiltInError(L"Type " + toS(val.name) + L" was not found.");
	}

	static void addBuiltIn(Engine &to, const BuiltInFunction *fn, Type *insertInto = null) {
		Named *into = null;
		vector<Value> params;
		bool typeMember = false;

		if (fn->pkg) {
			assert(insertInto == null);

			Auto<Name> pkg = parseSimpleName(to, fn->pkg);
			Package *p = to.package(pkg, true);
			if (!p)
				throw BuiltInError(L"Failed to locate package " + toS(fn->pkg));
			into = p;
		} else {
			Type *t = insertInto;
			if (!t)
				t = to.builtIn(fn->memberId);
			into = t;
			typeMember = true;

			// add the 'this' parameter
			params.push_back(Value::thisPtr(t));
		}

		Scope scope(into);
		Value result = findValue(scope, fn->result, to);

		params.reserve(fn->params.size());
		for (nat i = 0; i < fn->params.size(); i++)
			params.push_back(findValue(scope, fn->params[i], to));

		Auto<Function> toAdd;
		if (typeMember && fn->name == Type::DTOR) {
			toAdd = nativeDtor(to, params[0].type, fn->fnPtr);
		} else if (typeMember) {
			// Make sure we handle vtable calls correctly!
			toAdd = nativeMemberFunction(to, params[0].type, result, fn->name, params, fn->fnPtr);
		} else {
			toAdd = nativeFunction(to, result, fn->name, params, fn->fnPtr);
		}

		if (Package *p = as<Package>(into)) {
			p->add(toAdd.borrow());
		} else if (Type *t = as<Type>(into)) {
			t->add(toAdd.borrow());
		} else {
			assert(false);
		}
	}

	static void addToPkg(Engine &to, const BuiltInType *t) {
		Auto<Name> pkgName = parseSimpleName(to, t->pkg);
		Package *pkg = to.package(pkgName, true);
		if (!pkg)
			throw BuiltInError(L"Failed to locate package " + toS(t->pkg));

		Type *type = to.builtIn(t->typePtrId);
		pkg->add(type);
	}

	static void addHidden(Engine &e, Type *to, nat from) {
		for (const BuiltInFunction *fn = builtInFunctions(); fn->fnPtr; fn++) {
			if (fn->pkg == null && fn->memberId == from) {
				String name = fn->name;
				if (name == L"__ctor" || name == L"__dtor")
					continue;

				// Insert this function into ourselves!
				addBuiltIn(e, fn, to);
			}
		}

		const BuiltInType *p = findById(from);
		if (p->superMode == BuiltInType::superClass)
			addHidden(e, to, p->typePtrId);
	}

	static void addSuper(Engine &to, const BuiltInType *t) {

		Type *tc = to.builtIn(t->typePtrId);
		if (t->superMode == BuiltInType::superClass) {
			// Add as usual.
			Type *super = to.builtIn(t->super);
			if (!super)
				throw BuiltInError(L"Failed to locate super type with id " + toS(t->super));
			tc->setSuper(super);
		} else if (tc->flags & typeClass) {
			Type *obj = Object::stormType(to);
			Type *tObj = TObject::stormType(to);

			// We promised to set the super class earlier, so we need some default.
			if (tc == obj || tc == tObj)
				;
			else if (t->superMode == BuiltInType::superThread)
				tc->setSuper(tObj);
			else
				tc->setSuper(obj);
		}
	}

	static NamedThread *addThread(Engine &to, const BuiltInThread *t) {
		Auto<Name> pkgName = parseSimpleName(to, t->pkg);
		Package *pkg = to.package(pkgName, true);
		if (!pkg)
			throw BuiltInError(L"Failed to locate package " + toS(t->pkg));

		Thread *thread = t->decl->thread(to);
		Auto<NamedThread> created = CREATE(NamedThread, to, String(t->name), thread);
		pkg->add(created);
		return created.borrow();
	}

	static vector<NamedThread *> addThreads(Engine &to) {
		vector<NamedThread *> threads;
		for (const BuiltInThread *t = builtInThreads(); t->name; t++) {
			NamedThread *created = addThread(to, t);
			threads.push_back(created);
		}
		return threads;
	}

	static void addBuiltIn(Engine &to) {
		// This should be done first, since package lookup may require the use of as<>.
		for (const BuiltInType *t = builtInTypes(); t->name; t++) {
			addSuper(to, t);
		}
		for (const BuiltInType *t = builtInTypes(); t->name; t++) {
			addToPkg(to, t);
		}
		vector<NamedThread *> threads = addThreads(to);
		for (const BuiltInFunction *fn = builtInFunctions(); fn->fnPtr; fn++) {
			addBuiltIn(to, fn);
		}

		for (const BuiltInType *t = builtInTypes(); t->name; t++) {
			switch (t->superMode) {
			case BuiltInType::superHidden:
				// We should copy all the parent functions to ourselves!
				addHidden(to, to.builtIn(t->typePtrId), t->super);
				break;
			case BuiltInType::superThread:
				to.builtIn(t->typePtrId)->setThread(threads[t->super]);
				break;
			}
		}
	}

	void createStdTypes(Engine &to, vector<Auto<Type> > &cached) {
		nat count = builtInCount();
		const BuiltInType *b = builtInTypes();

		cached.resize(count);

		// The first type should be the Type-type, which requires special
		// treatment. (We do not have a Type to feed CREATE yet)
		const BuiltInType *tType = findById(0);
		if (tType == null || tType->name != String(L"Type"))
			throw BuiltInError(L"The first type is not Type.");
		cached[0] = Type::createType(to, L"Type", typeClass | typeManualSuper);

		// Now we can create the rest of all types.
		for (nat i = 0; i < count; i++) {
			const BuiltInType &t = b[i];
			nat id = t.typePtrId;

			if (id == 0)
				continue;

			if (cached.size() <= id)
				cached.resize(id + 1);

			// Here we're not giving the right size for other platforms than the current platform.
			cached[i] = CREATE(Type, to, t.name, TypeFlags(t.typeFlags) | typeManualSuper, Size(t.typeSize), t.cppVTable);
		}
		TODO(L"Find a way to compute the size of types for 64-bit as well!");
	}


	void addStdLib(Engine &to) {
		// Place core types in the core package.
		Auto<Name> coreName = CREATE(Name, to, L"core");
		Package *core = to.package(coreName, true);
		core->add(steal(intType(to)));
		core->add(steal(natType(to)));
		core->add(steal(byteType(to)));
		core->add(steal(boolType(to)));
		addArrayTemplate(core);
		core->add(steal(futureTemplate(to)));
		core->add(steal(cloneTemplate(to)));

		addBuiltIn(to);
	}

	nat maxVTableCount() {
		nat r = 0;

		for (const BuiltInType *t = builtInTypes(); t->name; t++) {
			if (t->typeFlags & typeValue)
				continue;

			if (t->cppVTable == null) {
				WARNING(L"Missing VTable for " << t->name);
			}
			r = max(r, code::vtableCount(t->cppVTable));
		}
		return r;
	}
}
