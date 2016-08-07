#include "stdafx.h"
#include "World.h"
#include "Exception.h"

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

World::World() : types(usingDecl), templates(usingDecl), threads(usingDecl) {
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
	if (!type)
		throw Error(L"The type storm::Type was not found!", SrcPos());

	struct pred {
		Type *type;

		pred(Type *t) : type(t) {}

		bool operator ()(const Auto<Type> &l, const Auto<Type> &r) const {
			// Always put 'type' first.
			if (l.borrow() == type)
				return true;
			if (r.borrow() == type)
				return false;

			// Then order by name.
			return l->name < r->name;
		}
	};

	types.sort(pred(type));
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

void World::resolveTypes() {
	for (nat i = 0; i < types.size(); i++) {
		types[i]->resolveTypes(*this);
	}
}

void World::prepare() {
	orderTypes();
	orderTemplates();
	orderThreads();

	resolveTypes();
}
