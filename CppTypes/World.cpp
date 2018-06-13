#include "stdafx.h"
#include "World.h"
#include "Exception.h"
#include "Config.h"

struct BuiltIn {
	const wchar_t *name;
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
	{ L"Double", Size::sDouble },
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

UnknownPrimitive *World::unknown(const String &name, const SrcPos &pos) {
	map<String, Auto<UnknownPrimitive>>::const_iterator i = unknownLookup.find(name);
	if (i != unknownLookup.end())
		return i->second.borrow();

	// Try to find it inside 'types'.
	UnknownPrimitive *found = null;
	for (nat i = 0; i < types.size(); i++) {
		if (Auto<UnknownPrimitive> u = types[i].as<UnknownPrimitive>()) {
			String last = u->name.last();
			unknownLookup[last] = u;

			if (last == name)
				found = u.borrow();
		}
	}

	if (found)
		return found;

	throw Error(L"Failed to find the unknown type for " + ::toS(name), pos);
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

			// Order by 'inheritanceDepth' to make the least specific types come first. This
			// improves the load performance inside CppLoader due to VTable creation taking less
			// time in this case.
			nat lDepth = l->inheritanceDepth();
			nat rDepth = r->inheritanceDepth();
			if (lDepth != rDepth)
				return lDepth < rDepth;

			// Then order by name.
			return l->name < r->name;
		}
	};

	types.sort(NamePred(type));
}

void World::orderFunctions() {
	struct pred {
		bool operator ()(const Function &l, const Function &r) const {
			// If the first parameter is named 'this', then sort on the inheritance depth of that parameter.
			bool lThis = !l.paramNames.empty() && l.paramNames[0] == L"this";
			bool rThis = !r.paramNames.empty() && r.paramNames[0] == L"this";
			if (lThis && rThis) {
				ResolvedType *lRes = findResolved(l.params[0].borrow());
				ResolvedType *rRes = findResolved(r.params[0].borrow());

				if (lRes && rRes) {
					// if (lRes->type->inheritanceDepth() != rRes->type->inheritanceDepth())
					// 	return lRes->type->inheritanceDepth() < rRes->type->inheritanceDepth();
				}
			} else if (lThis) {
				return false;
			} else if (rThis) {
				return true;
			}

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
	Auto<Doc> copy = new Doc(L"Copy constructor.");
	Auto<Doc> assign = new Doc(L"Assignment operator.");

	for (nat i = 0; i < types.size(); i++) {
		Type *t = types[i].borrow();
		t->resolveTypes(*this);

		if (Class *c = as<Class>(t)) {
			// Add the default copy-constructor to the type unless it is an actor.
			if (!c->isActor() && !c->external) {
				Auto<TypeRef> r = new NamedType(c->pos, L"void");
				Function f(c->name + Function::ctor, c->pkg, aPublic, c->pos, copy, r);
				f.isMember = true;
				f.params.push_back(new RefType(new ResolvedType(t)));
				f.paramNames.push_back(L"this");
				f.params.push_back(new RefType(makeConst(new ResolvedType(t))));
				f.paramNames.push_back(L"other");
				functions.push_back(f);
			}

			// Add the default assignment operator to the type if it is a value.
			if (c->valueType && !c->external) {
				Auto<TypeRef> r = new RefType(new ResolvedType(t));
				Function f(c->name + String(L"operator ="), c->pkg, aPublic, c->pos, assign, r);
				f.isMember = true;
				f.isConst = true;
				f.wrapAssign = true;
				f.params.push_back(r);
				f.paramNames.push_back(L"this");
				f.params.push_back(new RefType(makeConst(new ResolvedType(t))));
				f.paramNames.push_back(L"other");
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
	resolveTypes();

	orderTypes();
	orderTemplates();
	orderThreads();
	orderLicenses();
	orderFunctions();
}

