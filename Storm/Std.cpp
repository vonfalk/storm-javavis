#include "stdafx.h"
#include "Std.h"
#include "Lib/BuiltIn.h"
#include "Lib/Int.h"
#include "Lib/Bool.h"
#include "Exception.h"
#include "Function.h"
#include "Engine.h"
#include "Code/VTable.h"

namespace storm {

	static Value findValue(const Scope &src, const ValueRef &val) {
		if (val.name == null)
			return Value();

		Named *f = src.find(Name(val.name));
		if (Type *t = as<Type>(f))
			return Value(t, val.ref);

		throw BuiltInError(L"Type " + toS(val.name) + L" was not found.");
	}

	static void addBuiltIn(Engine &to, const BuiltInFunction *fn) {
		Named *into = null;
		NameLookup *top = null;
		vector<Value> params;

		if (!fn->typeMember) {
			Package *p = to.package(fn->pkg, true);
			if (!p)
				throw BuiltInError(L"Failed to locate package " + toS(fn->pkg));
			into = p;
			top = p;
		} else {
			Type *t = as<Type>(to.scope()->find(fn->pkg + Name(fn->typeMember)));
			into = t;
			top = as<NameLookup>(into);
			if (!into || !top)
				throw BuiltInError(L"Could not locate " + String(fn->typeMember) + L" in " + toS(fn->pkg));

			// add the 'this' parameter
			if (t->flags & typeClass) {
				params.push_back(Value(t));
			} else if (t->flags & typeValue) {
				params.push_back(Value(t).asRef());
			} else {
				assert(false);
			}
		}

		Scope scope(top);
		Value result = findValue(scope, fn->result);

		params.reserve(fn->params.size());
		for (nat i = 0; i < fn->params.size(); i++)
			params.push_back(findValue(scope, fn->params[i]));

		Auto<Function> toAdd;
		if (fn->typeMember) {
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
		Package *pkg = to.package(t->pkg, true);
		if (!pkg)
			throw BuiltInError(L"Failed to locate package " + toS(t->pkg));

		Type *type = to.builtIn(t->typePtrId);
		pkg->add(type);
	}

	static void addSuper(Engine &to, const BuiltInType *t) {
		if (t->super.empty())
			return;

		Type *tc = to.builtIn(t->typePtrId);
		Named *s = to.scope()->find(t->super);
		Type *super = as<Type>(s);
		if (!super)
			throw BuiltInError(L"Failed to locate super type " + toS(t->super));

		tc->setSuper(super);
	}

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

	static void addBuiltIn(Engine &to) {
		for (const BuiltInType *t = builtInTypes(); t->name; t++) {
			addToPkg(to, t);
		}
		for (const BuiltInType *t = builtInTypes(); t->name; t++) {
			addSuper(to, t);
		}
		for (const BuiltInFunction *fn = builtInFunctions(); fn->fnPtr; fn++) {
			addBuiltIn(to, fn);
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
		cached[0] = Type::createType(to, L"Type", typeClass);

		// Now we can create the rest of all types.
		for (nat i = 0; i < count; i++) {
			const BuiltInType &t = b[i];
			nat id = t.typePtrId;

			if (id == 0)
				continue;

			if (cached.size() <= id)
				cached.resize(id + 1);
			// Here we're not giving the right size for other platforms than the current platform.
			cached[i] = CREATE(Type, to, t.name, TypeFlags(t.typeFlags), Size(t.typeSize), t.cppVTable);
		}
		TODO(L"Find a way to compute the size of types for 64-bit as well!");
	}


	void addStdLib(Engine &to) {
		// Place core types in the core package.
		Package *core = to.package(Name(L"core"), true);
		core->add(steal(intType(to)));
		core->add(steal(natType(to)));
		core->add(steal(boolType(to)));

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
