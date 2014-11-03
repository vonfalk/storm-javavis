#include "stdafx.h"
#include "StdLib.h"
#include "Lib/BuiltIn.h"
#include "Lib/TypeClass.h"
#include "Lib/Int.h"
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

		NativeFunction *toAdd = new NativeFunction(result, fn->name, params, fn->fnPtr);
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

	static Type *addBuiltIn(Engine &to, const BuiltInType *t) {
		Package *pkg = to.package(t->pkg);
		if (!pkg)
			throw BuiltInError(L"Failed to locate package " + toS(t->pkg));

		Type *tc = new Type(to, t->name, typeClass);
		pkg->add(tc);
		return tc;
	}

	static void addSuper(Engine &to, const BuiltInType *t) {
		if (t->super.empty())
			return;

		Name fullName = t->pkg + Name(t->name);
		Named *n = to.scope()->find(fullName);
		Type *tc = as<Type>(n);
		if (!tc)
			throw BuiltInError(L"Failed to locate type " + toS(fullName));

		Named *s = Scope(tc).find(t->super);
		Type *super = as<Type>(s);
		if (!super)
			throw BuiltInError(L"Failed to locate super type " + toS(t->super));

		tc->setSuper(super);
	}

	static void addBuiltIn(Engine &to, vector<Type *> &cached) {
		cached.clear();
		for (const BuiltInType *t = builtInTypes(); t->name; t++) {
			cached.push_back(addBuiltIn(to, t));
		}
		for (const BuiltInType *t = builtInTypes(); t->name; t++) {
			addSuper(to, t);
		}
		for (const BuiltInFunction *fn = builtInFunctions(); fn->fnPtr; fn++) {
			addBuiltIn(to, fn);
		}
	}

	void addStdLib(Engine &to, vector<Type *> &cached, Type *&tType) {
		// Place core types in the core package.
		Package *core = to.package(Name(L"core"), true);
		core->add(intType(to));
		core->add(natType(to));

		// TODO: This should be handled normally later on.
		tType = typeType(to);
		core->add(tType);

		addBuiltIn(to, cached);
	}

}
