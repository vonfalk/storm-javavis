#include "stdafx.h"
#include "World.h"
#include "Tokenizer.h"

// Read a name.
static CppName parseName(Tokenizer &tok) {
	std::wostringstream out;
	out << tok.next().token;

	while (tok.more() && tok.skipIf(L"::")) {
		out << tok.next().token;
	}

	return CppName(out.str());
}

// Parse a type.
static void parseType(Tokenizer &tok, World &world, const CppName &inside) {
	Token name = tok.next();
	PVAR(name);
	Type type(inside + name.token, name.pos);

	if (tok.skipIf(L":")) {
		if (tok.skipIf(L"public")) {
			PVAR(parseName(tok));
		}

		// Skip any multi-inheritance.
		while (tok.peek().token != L"{")
			tok.skip();
	}

	tok.expect(L"{");
}

// Parse in the context of a namespace.
static void parseNamespace(Tokenizer &tok, World &world, const CppName &name) {
	nat depth = 0;

	while (tok.more()) {
		Token t = tok.next();

		if (t.token == L"class" || t.token == L"struct") {
			parseType(tok, world, name);
		} else if (t.token == L"namespace") {
			Token n = tok.next();
			if (n == L"{") {
				// Anonymous namespace.
				parseNamespace(tok, world, name);
			} else {
				tok.expect(L"{");
				parseNamespace(tok, world, name + n.token);
			}
		} else if (t.token == L"{") {
			// Any block we do not know about...
			depth++;
		} else if (t.token == L"}") {
			if (depth == 0)
				return;
			else
				depth--;
		} else {
			// TODO: Find any functions!
		}
	}
}

// Parse an entire header file.
static void parseFile(nat id, World &world) {
	Tokenizer tok(id, 0);

	parseNamespace(tok, world, CppName());
}

World parseWorld() {
	World world;

	for (nat i = 0; i < SrcPos::files.size(); i++) {
		parseFile(i, world);
	}

	return world;
}
