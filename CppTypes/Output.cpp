#include "stdafx.h"
#include "Output.h"
#include "CppName.h"
#include "World.h"
#include "Config.h"

/**
 * TODO: Store both 32 and 64-bit pointers!
 */


// Convert from a fully-qualified (type) name to a suitable variable name.
static String toVarName(const CppName &c) {
	String s = c;
	for (nat i = 0; i < s.size(); i++)
		if (s[i] == ':')
			s[i] = '_';
	return s;
}

// Convert a Size or an Offset to a string representation.
static String format(const Size &s) {
	std::wostringstream to;
	to << L"{ " << s.size32() << L", " << s.size64() << L", " << s.align32() << L", " << s.align64() << L" }";
	return to.str();
}

static String format(const Offset &o) {
	std::wostringstream to;
	to << L"{ " << o.v32() << L", " << o.v64() << L" }";
	return to.str();
}

static String offset(const Offset &o) {}

static void genIncludes(wostream &to, World &w) {
	// Include all relevant files.
	set<nat> files;
	for (nat i = 0; i < w.types.size(); i++) {
		files.insert(w.types[i]->pos.fileId);
	}

	// TODO: include all files containing functions we're interested in as well!

	for (set<nat>::iterator i = files.begin(); i != files.end(); ++i) {
		if (*i != SrcPos::invalid)
			to << L"#include \"" << SrcPos::files[*i].makeRelative(config.dir) << L"\"\n";
	}
}

static void genGlobals(wostream &to, World &w) {
	// Generate bodies for T::stormType():
	for (nat i = 0; i < w.types.size(); i++) {
		Type &t = *w.types[i];

		to << L"storm::Type *" << t.name << L"::stormType(Engine &e) { return e.cppType(" << i << L"); }\n";
		to << L"storm::Type *" << t.name << L"::stormType(const Object *o) { return o->engine().cppType(" << i << L"); }\n";
	}
}

static void genPtrOffsets(wostream &to, World &w) {
	for (nat i = 0; i < w.types.size(); i++) {
		Type &t = *w.types[i];
		to << L"static CppOffset " << toVarName(t.name) << L"_offset[] = { ";

		vector<Offset> o = t.ptrOffsets();
		for (nat i = 0; i < o.size(); i++) {
			to << format(o[i]) << L", ";
		}

		to << L"CppOffset::invalid };\n";
	}
}

static void genTypes(wostream &to, World &w) {
	for (nat i = 0; i < w.types.size(); i++) {
		Type &t = *w.types[i];
		to << L"{ ";

		// Name.
		to << L"L\"" << t.name.last() << L"\", ";

		// Parent class (if any).
		{
			Type *parent = null;
			if (Class *c = as<Class>(&t))
				parent = c->parentType;
			if (parent) {
				to << parent->id << L" /* " << parent->name << " */, ";
			} else {
				to << L"-1 /* NONE */, ";
			}
		}

		// Size.
		to << format(t.size()) << L", ";

		// Pointer offsets.
		to << toVarName(t.name) << L"_offset },\n";
	}
}

GenerateMap genMap() {
	struct E {
		wchar *tag;
		GenerateFn fn;
	};

	static E e[] = {
		{ L"INCLUDES", &genIncludes },
		{ L"GLOBALS", &genGlobals },
		{ L"PTR_OFFSETS", &genPtrOffsets },
		{ L"CPP_TYPES", &genTypes },
	};

	GenerateMap g;
	for (nat i = 0; i < ARRAY_COUNT(e); i++)
		g.insert(make_pair(e[i].tag, e[i].fn));

	return g;
}
