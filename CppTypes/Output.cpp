#include "stdafx.h"
#include "Output.h"
#include "CppName.h"
#include "World.h"
#include "Config.h"

// From Storm headers. Assumed to be 64-bits.
typedef long long int Long;

// Convert from a fully-qualified (type) name to a suitable variable name.
static String toVarName(const CppName &c) {
	String s = c;
	for (nat i = 0; i < s.size(); i++)
		if (s[i] == ':')
			s[i] = '_';
	return s;
}

// Name of the function for getting the C++ vtable for a type.
static String stormVTableName(const CppName &typeName) {
	return L"vtable_" + toVarName(typeName);
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

	for (nat i = 0; i < w.functions.size(); i++) {
		files.insert(w.functions[i].pos.fileId);
	}

	for (set<nat>::iterator i = files.begin(); i != files.end(); ++i) {
		if (*i != SrcPos::invalid)
			to << L"#include \"" << SrcPos::files[*i].makeRelative(config.cppOut.parent()) << L"\"\n";
	}
}

static void genPrimitiveGlobals(wostream &to, World &w) {
	// Generate bodies for T::stormType():
	for (nat i = 0; i < w.types.size(); i++) {
		Type *t = w.types[i].borrow();

		if (Primitive *p = as<Primitive>(t)) {
			to << L"const storm::Nat " << p->name << L"Id = " << i << L";\n";
		}
	}
}

static void genGlobals(wostream &to, World &w) {
	// Generate bodies for T::stormType():
	for (nat i = 0; i < w.types.size(); i++) {
		Type *t = w.types[i].borrow();

		if (Class *c = as<Class>(t)) {
			to << L"const storm::Nat " << c->name << L"::stormTypeId = " << i << L";\n";
			to << L"storm::Type *" << c->name << L"::stormType(storm::Engine &e) { return runtime::cppType(e, " << i << L"); }\n";
			to << L"storm::Type *" << c->name << L"::stormType(const storm::RootObject *o) { return runtime::cppType(o->engine(), " << i << L"); }\n";
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

static bool isType(World &w, Type &t) {
	for (Class *now = as<Class>(&t); now; now = as<Class>(now->parentType))
		if (now == w.types[0].borrow())
			return true;

	return false;
}

static bool hasVTable(const Type &t) {
	if (const Class *c = as<const Class>(&t)) {
		return !c->valueType;
	} else {
		return false;
	}
}

static void genVTableFns(wostream &to, World &w) {
	for (nat i = 0; i < w.types.size(); i++) {
		const Type &t = *w.types[i];
		if (!hasVTable(t))
			continue;

		to << L"extern \"C\" const void *" << stormVTableName(t.name) << L"();\n";
	}
}

static void genTypes(wostream &to, World &w) {
	for (nat i = 0; i < w.types.size(); i++) {
		Type &t = *w.types[i];
		Class *c = as<Class>(&t);
		Primitive *p = as<Primitive>(&t);
		to << L"{ ";

		// Name.
		to << L"L\"" << t.name.last() << L"\", ";

		// Package.
		if (t.pkg.empty() && config.compiler)
			PLN(t.pos << L": warning: placing types in the root package.");
		to << L"L\"" << t.pkg << L"\", ";

		// Parent class (if any).
		{
			Type *parent = null;
			Thread *thread = null;
			if (c) {
				parent = c->hiddenParent ? null : c->parentType;
				thread = c->threadType;
			}

			if (t.external) {
				to << L"CppType::superExternal, 0, ";
			} else if (Enum *e = as<Enum>(&t)) {
				if (e->bitmask)
					to << L"CppType::superBitmaskEnum, null, ";
				else
					to << L"CppType::superEnum, null, ";
			} else if (p) {
				to << L"CppType::superCustom, size_t(&" << p->generate << L"), ";
			} else if (thread) {
				to << L"CppType::superThread, " << thread->id << L" /* " << thread->name << L" */, ";
			} else if (parent) {
				if (isType(w, *parent))
					to << L"CppType::superClassType, ";
				else
					to << L"CppType::superClass, ";
				to << parent->id << L" /* " << parent->name << L" */, ";
			} else {
				to << L"CppType::superNone, 0, ";
			}
		}

		// Size.
		Size s = t.size();
		if (s == Size()) {
			throw Error(L"Zero-sized types are not yet properly supported!", t.pos);
		}
		to << format(s) << L", ";

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

		// VTable (if any).
		if (hasVTable(t)) {
			to << L"&" << stormVTableName(c->name);
		} else {
			to << L"null, ";
		}

		to << L"},\n";
	}
}

static TypeRef *findType(TypeRef *r, bool &ref, bool &maybe) {
	if (PtrType *t = as<PtrType>(r)) {
		ref = true;
		return findType(t->of.borrow(), ref, maybe);
	} else if (RefType *rt = as<RefType>(r)) {
		ref = true;
		return findType(rt->of.borrow(), ref, maybe);
	} else if (MaybeType *m = as<MaybeType>(r)) {
		maybe = true;
		return findType(m->of.borrow(), ref, maybe);
	} else {
		return r;
	}
}

static TypeRef *findType(TypeRef *r) {
	bool ref = false, maybe = false;
	return findType(r, ref, maybe);
}

static vector<Long> templateParamsId(ResolvedTemplateType *t) {
	vector<Long> out(t->params.size(), 0);

	for (nat i = 0; i < t->params.size(); i++) {
		TypeRef *type = t->params[i].borrow();
		if (as<VoidType>(type)) {
			out[i] = -2;
		} else if (ResolvedType *r = as<ResolvedType>(findType(type))) {
			out[i] = r->type->id;
		} else {
			throw Error(L"Unresolved type: " + ::toS(t->params[i]), t->pos);
		}
	}

	return out;
}

static String templateParamsName(ResolvedTemplateType *t) {
	std::wostringstream out;
	vector<Long> in = templateParamsId(t);

	out << L"template";
	for (nat i = 0; i < in.size(); i++)
		if (in[i] < 0)
			out << L"_null";
		else
			out << L"_" << in[i];

	return out.str();
}

static void templateParams(ResolvedTemplateType *t, wostream &to, set<String> &created) {
	if (t == null)
		return;

	String name = templateParamsName(t);
	if (created.count(name) == 0) {
		created.insert(name);

		vector<Long> ids = templateParamsId(t);

		to << L"static const size_t " << name << L"[] = { ";
		join(to, ids, L", ");
		to << L", -1 };\n";
	}
}

static bool genTypeRef(wostream &to, TypeRef *r, bool safe = false, bool skipExternal = false) {
	if (as<VoidType>(r)) {
		to << L"{ -1, null, false, false }";
		return true;
	}

	bool maybe = false;
	bool ref = false;
	r = findType(r, ref, maybe);

	if (ResolvedTemplateType *tt = as<ResolvedTemplateType>(r)) {
		to << L"{ " << tt->type->id << L", " << templateParamsName(tt);
	} else if (ResolvedType *rt = as<ResolvedType>(r)) {
		if (skipExternal && rt->type->external)
			return false;
		to << L"{ " << rt->type->id << L", null";
	} else if (safe) {
		return false;
	} else {
		throw Error(L"Type " + ::toS(*r) + L" not exported to Storm.", r->pos);
	}
	to << L", " << (ref   ? L"true" : L"false");
	to << L", " << (maybe ? L"true" : L"false") << L" }, ";
	return true;
}

static String genTypeRef(TypeRef *r, bool skipExternal = false) {
	std::wostringstream to;
	if (genTypeRef(to, r, true, skipExternal))
		return to.str();
	else
		return L"";
}

static void genTemplateArrays(wostream &to, World &w) {
	set<String> created;

	// 1: Generate any template-parameter-arrays needed.
	for (nat i = 0; i < w.functions.size(); i++) {
		Function &f = w.functions[i];

		for (nat p = 0; p < f.params.size(); p++) {
			TypeRef *r = findType(f.params[p].borrow());

			templateParams(as<ResolvedTemplateType>(r), to, created);
		}

		// Check the return value as well.
		templateParams(as<ResolvedTemplateType>(findType(f.result.borrow())), to, created);
	}

	// 2: Generate any template-parameter-arrays needed for variables.
	for (nat i = 0; i < w.types.size(); i++) {
		Class *c = as<Class>(w.types[i].borrow());
		if (!c)
			continue;

		for (nat j = 0; j < c->variables.size(); j++) {
			Variable &v = c->variables[j];
			if (v.access != aPublic)
				continue;

			TypeRef *r = findType(v.type.borrow());
			templateParams(as<ResolvedTemplateType>(r), to, created);
		}
	}
}

static void genFnParams(wostream &to, World &w) {
	// TODO: if this is slow, we can probably gain a lot of entries by reusing equal parameter lists.
	for (nat i = 0; i < w.functions.size(); i++) {
		Function &f = w.functions[i];

		if (f.params.empty())
			continue;

		if (f.params.size() == 1 && as<EnginePtrType>(f.params[0].borrow()) != null)
			continue;


		nat params = f.params.size();

		String name = f.name.last();
		if (name == L"operator ++" || name == L"operator --")
			// Ignore the dummy int-parameter if it is present.
			params = 1;

		to << L"static const CppTypeRef params_" << i << L"[] = { ";

		for (nat p = 0; p < params; p++) {
			TypeRef *r = f.params[p].borrow();

			// If 'EnginePtr' is the first parameter, then we should ignore it for now. It will be added by Storm later.
			if (p == 0 && as<EnginePtrType>(r))
				continue;

			genTypeRef(to, r);
		}

		to << L"{ -1, null, false } };\n";
	}

	// We only generate one for the empty parameter list.
	to << L"static const CppTypeRef params_empty[] = { { -1, null } };";
}

static void stormName(wostream &to, Function &f) {
	String name = f.name.last();
	if (name.size() > 9 && name.substr(0, 9) == L"operator ") {
		String op = name.substr(9).trim();
		if (op == L"++" || op == L"--") {
			if (f.params.size() == 1)
				op = op + L"*";
			else
				op = L"*" + op;
		}
		to << L"L\"" << op << L"\"";
	} else {
		to << L"L\"" << name << L"\"";
	}
}

static void genPtr(wostream &to, Function &fn) {
	String name = fn.name.last();
	if (name == Function::ctor) {
		to << L"address(&create" << fn.params.size() << L"<";
		to << *findType(fn.params[0].borrow());
		for (nat i = 1; i < fn.params.size(); i++)
			to << L", " << fn.params[i];
		to << L">)";
	} else if (name == Function::dtor) {
		to << L"address(&destroy<" << *findType(fn.params[0].borrow()) << L">)";
	} else if (fn.wrapAssign) {
		to << L"address(&assign<" << *findType(fn.params[0].borrow()) << L">)";
	} else if (fn.isMember) {
		to << L"address<" << fn.result << L" (CODECALL " << *findType(fn.params[0].borrow()) << L"::*)(";
		for (nat i = 1; i < fn.params.size(); i++) {
			if (i > 1)
				to << L", ";
			to << fn.params[i];
		}
		to << L")";
		if (fn.isConst)
			to << L" const";
		to << L">(&" << fn.name << L")";
	} else {
		to << L"address<" << fn.result << L" (CODECALL *)(";
		join(to, fn.params, L", ");
		to << L")>(&" << fn.name << L")";
	}
}

static void genFunctions(wostream &to, World &w) {
	for (nat i = 0; i < w.functions.size(); i++) {
		Function &f = w.functions[i];

		bool engineFn = f.params.size() >= 1 && as<EnginePtrType>(f.params[0].borrow()) != null;


		to << L"{ ";

		// Name.
		stormName(to, f);
		to << L", ";

		// Pkg.
		if (f.isMember) {
			to << L"null, ";
		} else {
			if (f.pkg.empty() && config.compiler)
				PLN(f.pos << L": warning: placing functions in the root package.");
			to << L"L\"" << f.pkg << L"\", ";
		}

		// Kind.
		if (f.isMember && f.castMember)
			to << L"CppFunction::fnCastMember, ";
		else if (f.isMember)
			to << L"CppFunction::fnMember, ";
		else if (engineFn)
			to << L"CppFunction::fnFreeEngine, ";
		else
			to << L"CppFunction::fnFree, ";

		// Thread id.
		if (f.isMember)
			to << L"-1, ";
		else if (f.threadType)
			to << f.threadType->id << L", ";
		else
			to << L"-1, ";

		// Pointer to the function.
		genPtr(to, f);
		to << L", ";

		// Parameters.
		if (f.params.empty())
			to << L"params_empty, ";
		else if (engineFn && f.params.size() == 1)
			to << L"params_empty, ";
		else
			to << L"params_" << i << L", ";

		// Result.
		genTypeRef(to, f.result.borrow());

		to << L" },\n";
	}
}

static void genVariables(wostream &to, World &w) {
	for (nat i = 0; i < w.types.size(); i++) {
		Class *c = as<Class>(w.types[i].borrow());
		if (!c)
			continue;
		if (c->external)
			continue;

		Size data = c->baseOffset();

		for (nat j = 0; j < c->variables.size(); j++) {
			Variable &v = c->variables[j];
			Offset offset = c->varOffset(j, data);

			// Only export public variables...
			if (v.access != aPublic)
				continue;

			String type = genTypeRef(v.type.borrow(), false);
			// ...which the Storm type system can handle. Ignores any external types.
			if (type.empty())
				continue;

			to << L"{ ";
			// Name.
			to << L"L\"" << v.name << L"\", ";

			// Owner id.
			to << c->id << L" /* " << c->name << L" */, ";

			// Type.
			to << type;

			// Offset.
			to << format(offset);

			to << L" },\n";
		}
	}
}

static void genEnumValues(wostream &to, World &w) {
	for (nat i = 0; i < w.types.size(); i++) {
		Enum *e = as<Enum>(w.types[i].borrow());
		if (!e)
			continue;
		if (e->external)
			continue;

		for (nat j = 0; j < e->members.size(); j++) {
			const String &member = e->members[j];

			to << L"{ ";

			// Name.
			to << L"L\"" << member << L"\", ";

			// Member of.
			to << e->id << L" /* " << e->name << L" */, ";

			// Value.
			to << e->name.parent() << L"::" << member;

			to << L" },\n";
		}
	}
}

static void genTemplateGlobals(wostream &to, World &w) {
	for (nat i = 0; i < w.templates.size(); i++) {
		Template &t = *w.templates[i];

		to << L"const storm::Nat " << t.name << L"Id = " << i << L";\n";
	}
}

static void genTemplates(wostream &to, World &w) {
	for (nat i = 0; i < w.templates.size(); i++) {
		Template &t = *w.templates[i];

		to << L"{ ";

		// Name.
		to << L"L\"" << t.name.last() << L"\", ";

		// Package.
		if (t.pkg.empty() && config.compiler)
			PLN(t.pos << L": warning: placing templates in the root package.");
		to << L"L\"" << t.pkg << L"\", ";

		// Generator function.
		if (config.compiler)
			to << L"&" << t.generator;
		else
			to << L"null";

		to << L" },\n";
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
		to << L"L\"" << t.name.last() << L"\", ";

		// Package.
		if (t.pkg.empty() && config.compiler)
			PLN(t.pos << L": warning: placing threads in the root package.");
		to << L"L\"" << t.pkg << L"\", ";

		// Declaration.
		to << L"&" << t.name << L"::decl, ";

		// External?
		to << (t.external ? L"true" : L"false");

		to << L" },\n";
	}
}

static void genRefPtrOffsets(wostream &to, World &w) {
	for (nat i = 0; i < w.types.size(); i++) {
		Type &t = *w.types[i];
		to << L"static const size_t " << toVarName(t.name) << L"_offset[] = { ";

		vector<ScannedVar> o = t.scannedVars();
		for (nat i = 0; i < o.size(); i++) {
			if (o[i].varName == L"") {
				to << L"-1, ";
			} else {
				to << L"OFFSET_OF(" << o[i].typeName << L", " << o[i].varName << L"), ";
			}
		}

		// For manually verifying no members are missing.
		// PLN(t.pos << L": Check contents!");
		// for (nat i = 0; i < o.size(); i++) {
		// 	if (o[i].typeName == t.name)
		// 		PLN(L"  " << o[i].varName);
		// }

		to << L"-1 };\n";
	}
}

static void genRefTypes(wostream &to, World &w) {
	for (nat i = 0; i < w.types.size(); i++) {
		Type &t = *w.types[i];
		to << L"{ sizeof(" << t.name << L"), ";
		to << toVarName(t.name) << L"_offset }, // #" << i << L"\n";
	}
}

static bool escape(wchar ch) {
	switch (ch) {
	case '"':
	case '\\':
	case '\n':
	case '\r':
	case '\t':
		return true;
	default:
		return ch > 0x7F;
	}
}

static void putHex(wostream &to, nat number, nat digits) {
	wchar lookup[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

	for (nat i = digits; i > 0; i--) {
		nat digit = number >> ((i - 1)*4);
		to << lookup[digit & 0x0F];
	}
}

static void escapeChar(wostream &to, const String &src, nat &pos) {
	wchar ch = src[pos];
	if (ch >= 0xD800 && ch <= 0xDFFF && pos + 1 <= src.size()) {
		// Decode surrogate pair...
		nat r = nat(ch & 0x3FF) << nat(10);
		r |= nat(src[++pos] & 0x3FF);
		r += 0x10000;
		to << L"\\U";
		putHex(to, r, 8);
	} else if (ch >= 0x7F) {
		to << L"\\u";
		putHex(to, ch, 4);
	} else {
		to << L"\\x";
		putHex(to, ch, 2);
	}
	// End the string and start a new one to avoid errors in escape sequence expansion...
	to << '"' << 'L' << '"';
}

static void outputStr(wostream &to, const String &str) {
	nat start = 0;
	while (start < str.size() && str[start] == '\n')
		start++;

	nat end = str.size();
	while (end > start && str[end-1] == '\n')
		end--;

	to << 'L' << '"';
	for (nat i = start; i < end; i++) {
		wchar ch = str[i];
		if (ch == '\r') {
			continue;
		} else if (ch == '\n') {
			to << L"\\n\"\nL\"";
		} else if (escape(ch)) {
			escapeChar(to, str, i);
		} else {
			to << ch;
		}
	}
	to << '"';
}

static void genLicenses(wostream &to, World &w) {
	for (nat i = 0; i < w.licenses.size(); i++) {
		const License &l = w.licenses[i];

		to << L"{ L\"" << l.id << L"\", ";
		to << L"L\"" << l.pkg << L"\", ";
		outputStr(to, l.title);
		to << L",\n";
		outputStr(to, l.body);
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
		{ L"PRIMITIVE_GLOBALS", &genPrimitiveGlobals },
		{ L"TYPE_GLOBALS", &genGlobals },
		{ L"PTR_OFFSETS", &genPtrOffsets },
		{ L"CPP_TYPES", &genTypes },
		{ L"VTABLE_DECLS", &genVTableFns },
		{ L"TEMPLATE_ARRAYS", &genTemplateArrays },
		{ L"FN_PARAMETERS", &genFnParams },
		{ L"CPP_FUNCTIONS", &genFunctions },
		{ L"CPP_VARIABLES", &genVariables },
		{ L"CPP_ENUM_VALUES", &genEnumValues },
		{ L"TEMPLATE_GLOBALS", &genTemplateGlobals },
		{ L"CPP_TEMPLATES", &genTemplates },
		{ L"THREAD_GLOBALS", &genThreadGlobals },
		{ L"CPP_THREADS", &genThreads },
		{ L"REF_PTR_OFFSETS", &genRefPtrOffsets },
		{ L"REF_TYPES", &genRefTypes },
		{ L"LICENSES", &genLicenses },
	};

	GenerateMap g;
	for (nat i = 0; i < ARRAY_COUNT(e); i++)
		g.insert(make_pair(e[i].tag, e[i].fn));

	return g;
}

static String vsVTableName(const CppName &typeName) {
	std::wostringstream r;

	r << L"??_7";

	nat last = typeName.size();
	for (nat i = typeName.size(); i > 0; i--) {
		if (typeName[i-1] != ':')
			continue;

		r << typeName.substr(i, last - i) << L"@";
		last = --i - 1;
	}

	r << typeName.substr(0, last);
	r << L"@@6B@";

	return r.str();
}

static void genVsX86Decl(wostream &to, World &w) {
	for (nat i = 0; i < w.types.size(); i++) {
		const Type &t = *w.types[i];
		if (!hasVTable(t))
			continue;

		to << vsVTableName(t.name) << L" proto syscall\n";
		to << stormVTableName(t.name) << L" proto\n\n";
	}
}

static void genVsX86Impl(wostream &to, World &w) {
	for (nat i = 0; i < w.types.size(); i++) {
		const Type &t = *w.types[i];
		if (!hasVTable(t))
			continue;

		String sName = stormVTableName(t.name);
		to << sName << L" proc\n";
		to << L"\tmov eax, " << vsVTableName(t.name) << L"\n";
		to << L"\tret\n";
		to << sName << L" endp\n\n";
	}
}

GenerateMap asmMap() {
	struct E {
		wchar *tag;
		GenerateFn fn;
	};

	static E e[] = {
		{ L"VS_X86_DECL", &genVsX86Decl },
		{ L"VS_X86_IMPL", &genVsX86Impl },
	};

	GenerateMap g;
	for (nat i = 0; i < ARRAY_COUNT(e); i++)
		g.insert(make_pair(e[i].tag, e[i].fn));

	return g;
}

