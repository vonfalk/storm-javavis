#include "stdafx.h"
#include "Output.h"
#include "CppName.h"
#include "World.h"
#include "Config.h"

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

	for (nat i = 0; i < w.threads.size(); i++) {
		files.insert(w.threads[i]->pos.fileId);
	}

	// TODO: include all files containing functions we're interested in as well!

	for (set<nat>::iterator i = files.begin(); i != files.end(); ++i) {
		if (*i != SrcPos::invalid)
			to << L"#include \"" << SrcPos::files[*i].makeRelative(config.cppOut.parent()) << L"\"\n";
	}
}

static void genGlobals(wostream &to, World &w) {
	// Generate bodies for T::stormType():
	for (nat i = 0; i < w.types.size(); i++) {
		Type *t = w.types[i].borrow();

		if (Class *c = as<Class>(t)) {
			to << L"storm::Type *" << c->name << L"::stormType(Engine &e) { return runtime::cppType(e, " << i << L"); }\n";
			to << L"storm::Type *" << c->name << L"::stormType(const Object *o) { return runtime::cppType(o->engine(), " << i << L"); }\n";
		}
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
		Class *c = as<Class>(&t);
		to << L"{ ";

		// Name.
		to << L"L\"" << t.name.last() << L"\", ";

		// Parent class (if any).
		{
			Type *parent = null;
			if (c)
				parent = c->hiddenParent ? null : c->parentType;
			if (parent) {
				to << parent->id << L" /* " << parent->name << " */, ";
			} else {
				to << L"-1 /* NONE */, ";
			}
		}

		// Size.
		to << format(t.size()) << L", ";

		// Pointer offsets.
		to << toVarName(t.name) << L"_offset, ";

		// Type flags.
		if (t.heapAlloc()) {
			to << L"typeClass, ";
		} else {
			to << L"typeValue, ";
		}

		// Destructor (if any).
		if (c != null && c->hasDtor()) {
			to << L"address(&destroy<" << c->name << L">), ";
		} else {
			to << L"null, ";
		}

		to << L"},\n";
	}
}

static void genThreadGlobals(wostream &to, World &w) {
	for (nat i = 0; i < w.threads.size(); i++) {
		Thread &t = *w.threads[i];

		to << L"const storm::Nat " << t.name << L"::identifier = " << i << L";\n";
	}
}

static void genThreads(wostream &to, World &w) {
	for (nat i = 0; i < w.threads.size(); i++) {
		Thread &t = *w.threads[i];
		to << L"{ ";

		// Name.
		to << L"L\"" << t.name.last() << L"\"";

		to << L" },\n";
	}
}

GenerateMap genMap() {
	struct E {
		wchar *tag;
		GenerateFn fn;
	};

	static E e[] = {
		{ L"INCLUDES", &genIncludes },
		{ L"TYPE_GLOBALS", &genGlobals },
		{ L"PTR_OFFSETS", &genPtrOffsets },
		{ L"CPP_TYPES", &genTypes },
		{ L"THREAD_GLOBALS", &genThreadGlobals },
		{ L"CPP_THREADS", &genThreads },
	};

	GenerateMap g;
	for (nat i = 0; i < ARRAY_COUNT(e); i++)
		g.insert(make_pair(e[i].tag, e[i].fn));

	return g;
}
