#include "stdafx.h"
#include "StdLib.h"
#include "BuiltIn.h"
#include "Exception.h"
#include "Function.h"

namespace storm {

	class BuiltInError : public Exception {
	public:
		BuiltInError(const String &msg) : msg(msg) {}
		virtual String what() const { return L"Error while loading built in functions: " + msg; }
	private:
		String msg;
	};

	static Value findValue(Scope *src, const Name &name) {
		Named *f = src->find(name);
		if (Type *t = as<Type>(f))
			return Value(t);

		throw BuiltInError(L"Type " + toS(name) + L" was not found.");
	}

	static void addBuiltIn(Engine &to, const BuiltInFunction *fn) {
		Named *into = null;
		Scope *scope = null;
		if (!fn->typeMember) {
			Package *p = to.package(fn->pkg, true);
			if (!p)
				throw BuiltInError(L"Failed to locate package " + toS(fn->pkg));
			into = p;
			scope = p;
		} else {
			into = to.scope()->find(fn->pkg + Name(fn->typeMember));
			scope = as<Scope>(into);
			if (!into || !scope)
				throw BuiltInError(L"Could not locate " + String(fn->typeMember) + L" in " + toS(fn->pkg));
		}

		Value result = null;
		vector<Value> params;
		Type *typeMember = null;

		result = findValue(scope, fn->result);
		params.reserve(fn->params.size());
		for (nat i = 0; i < fn->params.size(); i++)
			params.push_back(findValue(scope, fn->params[i]));

		if (Package *p = as<Package>(into)) {
			p->add(new Function(result, fn->name, params));
		} else if (Type *t = as<Type>(into)) {
			t->add(new Function(result, fn->name, params));
		} else {
			assert(false);
		}
	}

	static void addBuiltIn(Engine &to) {
		for (const BuiltInFunction *fn = builtInFunctions(); fn->fnPtr; fn++) {
			addBuiltIn(to, fn);
		}
	}

	void addStdLib(Engine &to) {
		// Place common types in the core package.
		Package *root = to.package(Name(L"core"));
		root->add(intType());
		root->add(natType());
		root->add(strType());

		addBuiltIn(to);

		PLN(*root);
	}

}
