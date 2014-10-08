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

	static Type *findType(Scope *src, const Name &name) {
		Named *f = src->find(name);
		if (Type *t = as<Type>(f))
			return t;

		throw BuiltInError(L"Type " + toS(name) + L" was not found.");
	}

	static void addBuiltIn(Engine &to, const BuiltInFunction *fn) {
		Package *p = to.package(fn->pkg, true);
		if (!p)
			throw BuiltInError(L"Failed to locate package " + toS(fn->pkg));

		Type *result = findType(p, fn->result);

		vector<Type*> params;
		params.reserve(fn->params.size());
		for (nat i = 0; i < fn->params.size(); i++)
			params.push_back(findType(p, fn->params[i]));

		p->add(new Function(result, fn->name, params));
	}

	static void addBuiltIn(Engine &to) {
		for (const BuiltInFunction *fn = builtInFunctions(); fn->fnPtr; fn++) {
			addBuiltIn(to, fn);
		}
	}

	void addStdLib(Engine &to) {
		// Place common types in the root package.
		Package *root = to.package(Name());
		root->add(intType());
		root->add(strType());

		addBuiltIn(to);

		PLN(*root);
	}

}
