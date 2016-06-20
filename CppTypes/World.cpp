#include "stdafx.h"
#include "World.h"
#include "Tokenizer.h"
#include "Exception.h"

void World::add(const Type &type) {
	if (typeLookup.count(type.name))
		throw Error(L"The type " + toS(type.name) + L" is defined twice!", type.pos);

	typeLookup.insert(make_pair(type.name, types.size()));
	types.push_back(type);
}

// Sort the types.
void World::orderTypes() {
	CppName type(L"storm::Type");
	if (typeLookup.count(type) == 0)
		throw Error(L"The type storm::Type was not found!", SrcPos());

	nat typePos = typeLookup[type];
	if (typePos != 0)
		std::swap(types[0], types[typePos]);

	struct pred {
		bool operator ()(const Type &l, const Type &r) const {
			return l.name < r.name;
		}
	};

	std::sort(types.begin() + 1, types.end(), pred());

	for (nat i = 0; i < types.size(); i++) {
		types[i].id = i;
		typeLookup[types[i].name] = i;
	}
}

Type *World::findTypeUnsafe(const CppName &name) {
	map<CppName, nat>::const_iterator i = typeLookup.find(name);
	if (i != typeLookup.end())
		return &types[i->second];

	for (nat j = 0; j < usingDecl.size(); j++) {
		i = typeLookup.find(usingDecl[j] + name);
		if (i != typeLookup.end())
			return &types[i->second];
	}

	return null;
}

Type *World::findType(const CppName &name, const SrcPos &pos) {
	Type *t = findTypeUnsafe(name);
	if (!t)
		throw Error(L"The type " + name + L" is not known to Storm.", pos);
	return t;
}

void World::resolveTypes() {
	for (nat i = 0; i < types.size(); i++) {
		types[i].resolveTypes(*this);
	}
}

void World::prepare() {
	orderTypes();

	resolveTypes();
}


// Namespace to add stuff to the global World.
class WorldNamespace : public Namespace {
public:
	WorldNamespace(World &to, const CppName &prefix) : to(to), prefix(prefix) {}

	World &to;
	CppName prefix;

	void add(const Variable &v) {
		// Add stuff to 'world'.
		PLN("Global variable " << v << " ignored.");
	}
};

// Read a name.
static CppName parseName(Tokenizer &tok) {
	std::wostringstream out;
	out << tok.next().token;

	while (tok.more() && tok.skipIf(L"::")) {
		out << tok.next().token;
	}

	return CppName(out.str());
}

// Read a type.
static Auto<CppType> parseType(Tokenizer &tok) {
	bool isConst = false;

	if (tok.skipIf(L"const"))
		isConst = true;

	Auto<CppType> type;
	if (tok.skipIf(L"MAYBE")) {
		tok.expect(L"(");
		type = new MaybeType(parseType(tok));
		tok.expect(L")");
	} else {
		SrcPos pos = tok.peek().pos;
		CppName n = parseName(tok);

		if (tok.skipIf(L"<")) {
			// Template parameters.
			Auto<TemplateType> t = new TemplateType(pos, n);

			if (!tok.skipIf(L">")) {
				t->params.push_back(parseType(tok));
				while (tok.skipIf(L","))
					t->params.push_back(parseType(tok));
				tok.expect(L">");
			}

			type = t;
		} else {
			type = new NamedType(pos, n);
		}
	}

	if (isConst)
		type->constType = true;

	while (tok.more()) {
		if (tok.skipIf(L"const"))
			type->constType = true;

		Token modifiers = tok.peek();
		bool wrong = false;
		Auto<CppType> nType = type;
		for (nat i = 0; i < modifiers.token.size(); i++) {
			if (modifiers.token[i] == '*') {
				nType = new PtrType(nType);
			} else if (modifiers.token[i] == '&') {
				nType = new RefType(nType);
			} else {
				wrong = true;
			}
		}

		if (wrong)
			break;

		tok.next();
		type = nType;
	}

	return type;
}

// Parse an unknown block.
static void parseBlock(Tokenizer &tok) {
	while (tok.more()) {
		Token t = tok.next();
		if (t.token == L"{")
			parseBlock(tok);
		if (t.token == L"}")
			break;
	}
}

// Parse a variable or function.
static void parseMember(Tokenizer &tok, Namespace &addTo) {
	if (!tok.more() || tok.skipIf(L";"))
		return;

	// First, we should have a type.
	Auto<CppType> type;
	if (tok.skipIf(L"~")) {
		type = new NamedType(tok.peek().pos, L"void");
	} else {
		type = parseType(tok);
	}

	// Something else, skip.
	if (!tok.more() || tok.skipIf(L";"))
		return;

	Token name(L"", type->pos);
	if (tok.peek().token != L"(") {
		// Then, we should have the member's name.
		name = tok.next();
		if (name.token == L"operator")
			name.token += L" " + tok.next().token;
	} else {
		// Constructor
		type = new NamedType(name.pos, L"void");
		name.token = L"ctor";
	}

	if (tok.skipIf(L"(")) {
		// Function.
		PLN(tok.peek().pos << ": function");

		if (!tok.skipIf(L")")) {
			parseType(tok);
			tok.skip(); // Parameter name.

			while (tok.more() && tok.skipIf(L",")) {
				parseType(tok);
				tok.skip();
			}

			tok.expect(L")");
		}

		tok.skipIf(L"const");
	} else if (tok.skipIf(L"[")) {
		throw Error(L"C-style arrays not supported in classes exposed to Storm.", name.pos);
	} else if (tok.skipIf(L";")) {
		// Variable.
		addTo.add(Variable(CppName(name.token), type, name.pos));
	} else {
		// Unknown construct; ignore it.
	}
}

// Parse a type.
static void parseTypeDecl(Tokenizer &tok, World &world, const CppName &inside) {
	Token name = tok.next();

	// Forward-declaration?
	if (tok.peek().token == L";")
		return;

	CppName parent;
	if (tok.skipIf(L":")) {
		if (tok.skipIf(L"public"))
			parent = parseName(tok);

		// Skip any multi-inheritance or private inheritance.
		while (tok.peek().token != L"{")
			tok.skip();
	}

	tok.expect(L"{");

	bool value = false;
	// Are we interested in this class at all?
	if (tok.skipIf(L"STORM_CLASS")) {
		value = false;
	} else if (tok.skipIf(L"STORM_VALUE")) {
		value = true;
	} else {
		// No. Ignore the rest of the block!
		parseBlock(tok);
		return;
	}
	tok.expect(L";");

	Type type(inside + name.token, name.pos, value);
	type.parent = parent;

	nat depth = 0;

	while (tok.more()) {
		Token t = tok.peek();

		if (t.token == L"{") {
			tok.skip();
			parseBlock(tok);
		} else if (t.token == L"}") {
			tok.skip();
			break;
		} else if (t.token == L"public") {
			tok.skip();
			tok.expect(L":");
		} else if (t.token == L"private") {
			tok.skip();
			tok.expect(L":");
		} else if (t.token == L"protected") {
			tok.skip();
			tok.expect(L":");
		} else if (t.token == L"static") {
			// Ignore static members. Maybe we should warn about this...
			TODO(L"Warn about static members?");
			while (!tok.skipIf(L";"))
				tok.skip();
		} else if (t.token == L"inline") {
			tok.skip();
		} else if (t.token == L"virtual") {
			// TODO: pass this on.
			tok.skip();
		} else {
			parseMember(tok, type);
		}
	}

	world.add(type);
}

// Parse in the context of a namespace.
static void parseNamespace(Tokenizer &tok, World &world, const CppName &name) {
	while (tok.more()) {
		Token t = tok.peek();

		if (t.token == L"class" || t.token == L"struct") {
			tok.skip();
			parseTypeDecl(tok, world, name);
		} else if (t.token == L"namespace") {
			tok.skip();
			Token n = tok.next();
			if (n == L"{") {
				// Anonymous namespace.
				parseNamespace(tok, world, name);
			} else {
				tok.expect(L"{");
				parseNamespace(tok, world, name + n.token);
			}
		} else if (t.token == L"{") {
			tok.skip();
			parseBlock(tok);
		} else if (t.token == L"}") {
			tok.skip();
			break;
		} else {
			parseMember(tok, WorldNamespace(world, name));
		}
	}
}

// Parse an entire header file.
static void parseFile(nat id, World &world) {
	Tokenizer tok(id);

	parseNamespace(tok, world, CppName());
}

World parseWorld() {
	World world;

	for (nat i = 0; i < SrcPos::files.size(); i++) {
		parseFile(i, world);
	}

	return world;
}
