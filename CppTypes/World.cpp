#include "stdafx.h"
#include "World.h"
#include "Tokenizer.h"
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

World::World() : types(usingDecl), threads(usingDecl) {
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
	orderThreads();

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
		PLN(L"Global variable " << v << L" ignored.");
	}

	void add(const Function &f) {
		PLN(L"Global exported function " << f << L" ignored.");
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
static Auto<TypeRef> parseTypeRef(Tokenizer &tok) {
	bool isConst = false;

	if (tok.skipIf(L"const"))
		isConst = true;

	Auto<TypeRef> type;
	if (tok.skipIf(L"MAYBE")) {
		tok.expect(L"(");
		type = new MaybeType(parseTypeRef(tok));
		tok.expect(L")");
	} else if (tok.skipIf(L"UNKNOWN")) {
		tok.expect(L"(");
		Token kind = tok.next();
		tok.expect(L")");
		type = new UnknownType(kind.token, parseTypeRef(tok));
	} else {
		SrcPos pos = tok.peek().pos;
		CppName n = parseName(tok);
		// pos = tok.pos();

		if (tok.skipIf(L"<")) {
			// Template parameters.
			Auto<TemplateType> t = new TemplateType(pos, n);

			if (!tok.skipIf(L">")) {
				t->params.push_back(parseTypeRef(tok));
				while (tok.skipIf(L","))
					t->params.push_back(parseTypeRef(tok));
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
		Auto<TypeRef> nType = type;
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

	Token name(L"", SrcPos());
	bool exportFn = tok.skipIf(L"STORM_CTOR");

	bool isVirtual = tok.skipIf(L"virtual");
	tok.skipIf(L"inline"); // The combination 'virtual inline' is insane, but we'll roll with it...

	// First, we should have a type.
	Auto<TypeRef> type;
	if (tok.skipIf(L"~")) {
		type = new NamedType(tok.peek().pos, L"void");
		tok.skip(); // Ignore the name of the destructor.
		name.token = Function::dtor;
	} else {
		type = parseTypeRef(tok);
	}

	// Something else, skip.
	if (!tok.more() || tok.skipIf(L";"))
		return;

	// Is this function supposed to be exported to Storm?
	exportFn |= tok.skipIf(L"STORM_FN");

	name.pos = type->pos;
	if (tok.peek().token != L"(") {
		// Then, we should have the member's name.
		name = tok.next();
		if (name.token == L"operator") {
			// < and > are tokenized one at a time.
			if (tok.skipIf(L"<")) {
				tok.expect(L"<");
				name.token += L" <<";
			} else if (tok.skipIf(L">")) {
				tok.expect(L">");
				name.token += L" >>";
			} else {
				name.token += L" " + tok.next().token;
			}
		}
	} else if (name.token != Function::dtor) {
		// Constructor
		type = new NamedType(name.pos, L"void");
		name.token = Function::ctor;
	}

	if (tok.skipIf(L"(")) {
		// Function.
		Function f(CppName(name.token), name.pos, type);
		f.isVirtual = isVirtual;

		if (!tok.skipIf(L")")) {
			f.params.push_back(parseTypeRef(tok));
			tok.skip(); // Parameter name.

			while (tok.more() && tok.skipIf(L",")) {
				f.params.push_back(parseTypeRef(tok));
				tok.skip();
			}

			tok.expect(L")");
		}

		if (tok.skipIf(L"const"))
			f.isConst = true;

		// Save if 'exportFn' is there.
		if (exportFn || f.name == Function::dtor) {
			addTo.add(f);
		}
	} else if (tok.skipIf(L"[")) {
		throw Error(L"C-style arrays not supported in classes exposed to Storm.", name.pos);
	} else if (tok.skipIf(L";")) {
		// Variable.
		addTo.add(Variable(CppName(name.token), type, name.pos));
	} else {
		// Unknown construct; ignore it.
	}
}

// Parse an enum declaration.
static void parseEnum(Tokenizer &tok, World &world, const CppName &inside) {
	Token name = tok.next();

	// Forward-declaration?
	if (tok.skipIf(L";"))
		return;

	tok.expect(L"{");

	// Add the type.
	CppName fullName = inside + name.token;
	Auto<Enum> type = new Enum(fullName, name.pos);

	do {
		Token member = tok.next();
		if (member.token == L"}")
			break;

		type->members.push_back(member.token);

		if (tok.skipIf(L"=")) {
			// Some expression for initialization. Skip it.
			while (tok.peek().token != L"," && tok.peek().token != L"}")
				tok.next();
		}

	} while (tok.skipIf(L","));
	tok.skipIf(L"}");
	tok.expect(L";");

	world.add(type);
}

// Parse the parent class of a type.
static Auto<Class> parseParent(Tokenizer &tok, const CppName &fullName, const SrcPos &pos) {
	Auto<Class> result = new Class(fullName, pos);
	if (!tok.skipIf(L":"))
		return result;

	if (tok.skipIf(L"public")) {
		if (tok.skipIf(L"STORM_HIDDEN")) {
			tok.expect(L"(");
			result->parent = parseName(tok);
			result->hiddenParent = true;
			tok.expect(L")");
		} else if (tok.skipIf(L"ObjectOn")) {
			tok.expect(L"<");
			result->parent = CppName(L"storm::TObject");
			result->hiddenParent = false;
			result->thread = parseName(tok);
			tok.expect(L">");
		} else {
			result->parent = parseName(tok);
			result->hiddenParent = false;
		}
	}

	// Skip any multi-inheritance or private inheritance.
	while (tok.peek().token != L"{")
		tok.skip();

	return result;
}

// Parse a type.
static void parseType(Tokenizer &tok, World &world, const CppName &inside) {
	Token name = tok.next();

	// Forward-declaration?
	if (tok.skipIf(L";"))
		return;

	CppName fullName = inside + name.token;
	Auto<Class> type = parseParent(tok, fullName, name.pos);

	tok.expect(L"{");

	// Are we interested in this class at all?
	if (tok.skipIf(L"STORM_CLASS")) {
		type->valueType = false;
	} else if (tok.skipIf(L"STORM_VALUE")) {
		type->valueType = true;
	} else {
		// No. Ignore the rest of the block!
		parseBlock(tok);
		return;
	}

	tok.expect(L";");

	while (tok.more()) {
		Token t = tok.peek();

		if (t.token == L"{") {
			tok.skip();
			parseBlock(tok);
		} else if (t.token == L"}") {
			tok.skip();
			break;
		} else if (t.token == L"class" || t.token == L"struct") {
			tok.skip();
			parseType(tok, world, fullName);
		} else if (t.token == L"extern" || t.token == L"static") {
			tok.skip();
		} else if (t.token == L"enum") {
			tok.skip();
			parseEnum(tok, world, fullName);
		} else if (t.token == L"template") {
			// Skip until we find a {, and skip the body as well.
			while (tok.skipIf(L"{"))
				tok.skip();
			parseBlock(tok);
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
		} else if (t.token == L"friend") {
			while (!tok.skipIf(L";"))
				tok.skip();
		} else if (t.token == L"using" || t.token == L"typedef") {
			while (!tok.skipIf(L";"))
				tok.skip();
		} else if (t.token == L"inline") {
			tok.skip();
		} else {
			parseMember(tok, *type);
		}
	}

	world.add(type);

	tok.expect(L";");
}

// Parse in the context of a namespace.
static void parseNamespace(Tokenizer &tok, World &world, const CppName &name) {
	while (tok.more()) {
		Token t = tok.peek();

		if (t.token == L"class" || t.token == L"struct") {
			tok.skip();
			parseType(tok, world, name);
		} else if (t.token == L"enum") {
			tok.skip();
			parseEnum(tok, world, name);
		} else if (t.token == L"template") {
			// Skip until we find a {, and skip the body as well.
			while (tok.skipIf(L"{"))
				tok.skip();
			parseBlock(tok);
		} else if (t.token == L"extern") {
			tok.skip();
			tok.skipIf(L"\"C\""); // for 'extern "C" {}'
		} else if (t.token == L"static") {
			tok.skip();
		} else if (t.token == L"using" || t.token == L"typedef") {
			while (!tok.skipIf(L";"))
				tok.skip();
		} else if (t.token == L"STORM_PKG") {
			tok.next();
			tok.expect(L"(");
			// TODO: Take care of this name properly!
			tok.next();
			while (tok.skipIf(L"."))
				tok.next();
			tok.expect(L")");
			tok.expect(L";");
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
		} else if (t.token == L"BITMASK_OPERATORS") {
			tok.skip();
			tok.expect(L"(");
			Token n = tok.next();
			Type *t = world.types.find(CppName(n.token), name, n.pos);
			if (Enum *e = as<Enum>(t)) {
				e->bitmask = true;
			} else {
				throw Error(L"The type " + n.token + L" is not an enum.", n.pos);
			}
			tok.expect(L")");
			tok.expect(L";");
		} else if (t.token == L"STORM_THREAD") {
			tok.skip();
			tok.expect(L"(");
			Token tName = tok.next();
			Auto<Thread> t = new Thread(name + tName.token, tName.pos);
			world.threads.insert(t);
			tok.expect(L")");
			tok.expect(L";");
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
