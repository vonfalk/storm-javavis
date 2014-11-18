#include "stdafx.h"
#include "Std.h"
#include "Lib/BuiltIn.h"
#include "Lib/Int.h"
#include "Lib/Bool.h"
#include "Exception.h"
#include "Function.h"
#include "Engine.h"

namespace storm {

	static Value findValue(const Scope &src, const Name &name) {
		Named *f = src.find(name);
		if (Type *t = as<Type>(f))
			return Value(t);

		throw BuiltInError(L"Type " + toS(name) + L" was not found.");
	}

	static void addBuiltIn(Engine &to, const BuiltInFunction *fn) {
		Named *into = null;
		Scope scope(null);
		if (!fn->typeMember) {
			Package *p = to.package(fn->pkg, true);
			if (!p)
				throw BuiltInError(L"Failed to locate package " + toS(fn->pkg));
			into = p;
			scope.top = p;
		} else {
			into = to.scope()->find(fn->pkg + Name(fn->typeMember));
			scope.top = as<NameLookup>(into);
			if (!into || !scope.top)
				throw BuiltInError(L"Could not locate " + String(fn->typeMember) + L" in " + toS(fn->pkg));
		}

		Value result;
		vector<Value> params;

		if (fn->typeMember) {
			if (fn->name == Type::CTOR)
				;
			else if (Type *t = as<Type>(into))
				params.push_back(Value(as<Type>(t)));
			else
				assert(false);
		}

		if (fn->result.any())
			result = findValue(scope, fn->result);
		params.reserve(fn->params.size());
		for (nat i = 0; i < fn->params.size(); i++)
			params.push_back(findValue(scope, fn->params[i]));

		NativeFunction *toAdd = CREATE(NativeFunction, to, result, fn->name, params, fn->fnPtr);
		try {
			if (Package *p = as<Package>(into)) {
				p->add(toAdd);
			} else if (Type *t = as<Type>(into)) {
				t->add(toAdd);
			} else {
				delete toAdd;
				assert(false);
			}
		} catch (...) {
			delete toAdd;
			throw;
		}
	}

	static void addToPkg(Engine &to, const BuiltInType *t) {
		Package *pkg = to.package(t->pkg, true);
		if (!pkg)
			throw BuiltInError(L"Failed to locate package " + toS(t->pkg));

		Type *type = to.builtIn(t->typePtrId);
		type->addRef();
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
			cached[i] = CREATE(Type, to, to, t.name, TypeFlags(t.typeFlags), t.typeSize);
		}
	}


	void addStdLib(Engine &to) {
		// Place core types in the core package.
		Package *core = to.package(Name(L"core"), true);
		core->add(intType(to));
		core->add(natType(to));
		core->add(boolType(to));

		addBuiltIn(to);
	}

}
