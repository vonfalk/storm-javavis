#include "stdafx.h"
#include "World.h"
#include "Exception.h"
#include "Config.h"

struct BuiltIn {
	wchar *name;
	const Size &size;
};

static BuiltIn builtIn[] = {
	{ L"bool", Size::sByte },
	{ L"Bool", Size::sByte },
	{ L"int", Size::sInt },
	{ L"nat", Size::sNat },
	{ L"Int", Size::sInt },
	{ L"Nat", Size::sNat },
	{ L"char", Size::sChar },
	{ L"byte", Size::sByte },
	{ L"Byte", Size::sByte },
	{ L"Long", Size::sLong },
	{ L"Word", Size::sWord },
	{ L"Float", Size::sFloat },
	{ L"size_t", Size::sPtr },
};

World::World() : types(usingDecl, &aliases), templates(usingDecl), threads(usingDecl) {
	for (nat i = 0; i < ARRAY_COUNT(::builtIn); i++) {
		builtIn.insert(make_pair(::builtIn[i].name, ::builtIn[i].size));
	}
}

void World::add(Auto<Type> type) {
	types.insert(type);
}

// Sort the types.
void World::orderTypes() {
	Type *type = types.findUnsafe(CppName(L"storm::Type"), CppName());
	if (!type && config.compiler)
		throw Error(L"The type storm::Type was not found! Are you really compiling the compiler?", SrcPos());

	struct NamePred {
		Type *type;

		NamePred(Type *type) : type(type) {}

		bool operator ()(const Auto<Type> &l, const Auto<Type> &r) const {
			if (l.borrow() == r.borrow())
				return false;

			// Always put 'type' first.
			if (l.borrow() == type)
				return true;
			if (r.borrow() == type)
				return false;

			// Then order by package first, then by name. This is so any nested classes shall have
			// their outer class appear before them.
			if (l->pkg != r->pkg)
				return l->pkg < r->pkg;

			// Then order by name.
			return l->name < r->name;
		}
	};

	types.sort(NamePred(type));
}

void World::orderFunctions() {
	struct pred {
		bool operator ()(const Function &l, const Function &r) const {
			if (l.name != r.name)
				return l.name < r.name;
			// Not entirely deterministic, but good enough for us.
			return l.params.size() < r.params.size();
		}
	};

	sort(functions.begin(), functions.end(), pred());
}

void World::orderTemplates() {
	struct pred {
		bool operator ()(const Auto<Template> &l, const Auto<Template> &r) const {
			return l->name < r->name;
		}
	};

	templates.sort(pred());
}

void World::orderThreads() {
	struct pred {
		bool operator ()(const Auto<Thread> &l, const Auto<Thread> &r) const {
			return l->name < r->name;
		}
	};

	threads.sort(pred());
}

void World::orderLicenses() {
	struct pred {
		bool operator ()(const License &l, const License &r) const {
			if (l.id == r.id)
				return l.id < r.id;
			return l.pkg < r.pkg;
		}
	};

	sort(licenses.begin(), licenses.end(), pred());
}

void World::resolveTypes() {
	for (nat i = 0; i < types.size(); i++) {
		Type *t = types[i].borrow();
		t->resolveTypes(*this);

		if (Class *c = as<Class>(t)) {
			// Add the default copy-constructor to the type unless it is an actor.
			if (!c->isActor() && !c->external) {
				Auto<TypeRef> r = new NamedType(c->pos, L"void");
				Function f(c->name + Function::ctor, c->pkg, aPublic, c->pos, r);
				f.isMember = true;
				f.params.push_back(new RefType(new ResolvedType(t)));
				f.params.push_back(new RefType(makeConst(new ResolvedType(t))));
				functions.push_back(f);
			}

			// Add the default assignment operator to the type if it is a value.
			if (c->valueType && !c->external) {
				Auto<TypeRef> r = new RefType(new ResolvedType(t));
				Function f(c->name + String(L"operator ="), c->pkg, aPublic, c->pos, r);
				f.isMember = true;
				f.isConst = true;
				f.wrapAssign = true;
				f.params.push_back(r);
				f.params.push_back(new RefType(makeConst(new ResolvedType(t))));
				functions.push_back(f);
			}
		}
	}

	for (nat i = 0; i < functions.size(); i++) {
		Function &fn = functions[i];
		fn.resolveTypes(*this, fn.name.parent());
	}
}

void World::prepare() {
	orderTypes();
	orderTemplates();
	orderThreads();
	orderLicenses();

	resolveTypes();
	orderFunctions();
}

