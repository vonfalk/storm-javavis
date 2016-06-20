#include "stdafx.h"
#include "Output.h"
#include "CppName.h"
#include "World.h"

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

static void genIncludes(wostream &to, World &w) {}

static void genPtrOffsets(wostream &to, World &w) {
	for (nat i = 0; i < w.types.size(); i++) {
		Type &t = w.types[i];
		to << L"static size_t " << toVarName(t.name) << L"_offset[] = { ";

		vector<Offset> o = t.ptrOffsets();
		for (nat i = 0; i < o.size(); i++) {
			to << o[i].current() << L", ";
		}

		to << L"CppType::invalidOffset };\n";
	}
}

static void genTypes(wostream &to, World &w) {
	for (nat i = 0; i < w.types.size(); i++) {
		Type &t = w.types[i];
		to << L"{ ";

		// Name.
		to << L"L\"" << t.name.last() << L"\", ";

		// Parent class (if any).
		if (t.parentType) {
			to << t.parentType->id << L" /* " << t.parentType->name << " */, ";
		} else {
			to << L"-1 /* NONE */, ";
		}

		// Size.
		to << t.size().current() << L", ";

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
		{ L"PTR_OFFSETS", &genPtrOffsets },
		{ L"CPP_TYPES", &genTypes },
	};

	GenerateMap g;
	for (nat i = 0; i < ARRAY_COUNT(e); i++)
		g.insert(make_pair(e[i].tag, e[i].fn));

	return g;
}
