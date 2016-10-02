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

static void genTypes(wostream &to, World &w) {
	for (nat i = 0; i < w.types.size(); i++) {
		Type &t = *w.types[i];
		Class *c = as<Class>(&t);
		Primitive *p = as<Primitive>(&t);
		to << L"{ ";

		// Name.
		to << L"L\"" << t.name.last() << L"\", ";

		// Package.
		if (t.pkg.empty())
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

			if (p) {
				to << L"CppType::superCustom, size_t(&" << p->generate << L"), ";
			} else if (thread) {
				to << L"CppType::superThread, " << thread->id << L" /* " << thread->name << L" */, ";
			} else if (parent) {
				to << L"CppType::superClass, " << parent->id << L" /* " << parent->name << L" */, ";
			} else {
				to << L"CppType::superNone, 0, ";
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

static TypeRef *findType(TypeRef *ref) {
	if (PtrType *t = as<PtrType>(ref)) {
		return findType(t->of.borrow());
	} else if (RefType *r = as<RefType>(ref)) {
		return findType(r->of.borrow());
	} else if (MaybeType *m = as<MaybeType>(ref)) {
		return findType(m->of.borrow());
	} else {
		return ref;
	}
}

static vector<nat> templateParamsId(ResolvedTemplateType *t) {
	vector<nat> out(t->params.size(), 0);

	for (nat i = 0; i < t->params.size(); i++) {
		if (ResolvedType *r = as<ResolvedType>(findType(t->params[i].borrow()))) {
			out[i] = r->type->id;
		} else {
			throw Error(L"Unresolved type: " + ::toS(t->params[i]), t->pos);
		}
	}

	return out;
}

static String templateParamsName(ResolvedTemplateType *t) {
	std::wostringstream out;
	vector<nat> in = templateParamsId(t);

	out << L"template";
	for (nat i = 0; i < in.size(); i++)
		out << L"_" << in[i];

	return out.str();
}

static void templateParams(ResolvedTemplateType *t, wostream &to, set<String> &created) {
	if (t == null)
		return;

	String name = templateParamsName(t);
	if (created.count(name) == 0) {
		created.insert(name);

		vector<nat> ids = templateParamsId(t);

		to << L"static const size_t " << name << L"[] = { ";
		join(to, ids, L", ");
		to << L", -1 };\n";
	}
}

static void genTypeRef(wostream &to, TypeRef *r) {
	if (NamedType *nt = as<NamedType>(r)) {
		if (nt->name == L"void") {
			to << L"{ -1, null, false }";
			return;
		}
	}

	bool maybe = as<MaybeType>(r) != null;
	r = findType(r);

	if (ResolvedTemplateType *tt = as<ResolvedTemplateType>(r)) {
		to << L"{ " << tt->type->id << L", " << templateParamsName(tt);
	} else if (ResolvedType *rt = as<ResolvedType>(r)) {
		to << L"{ " << rt->type->id << L", null";
	} else {
		throw Error(L"Type " + ::toS(*r) + L" not exported to Storm.", r->pos);
	}
	to << L", " << (maybe ? L"true" : L"false") << L" }, ";
}

static void genFnParams(wostream &to, World &w) {
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

	// 2: Generate the parameter arrays.
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
		if (f.isMember)
			to << L"null, ";
		else
			to << L"L\"" << f.pkg << L"\", ";

		// Kind.
		if (f.isMember)
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
		if (t.pkg.empty())
			PLN(t.pos << L": warning: placing templates in the root package.");
		to << L"L\"" << t.pkg << L"\", ";

		// Generator function (TODO: only emit for the compiler).
		to << L"&" << t.generator << L" ";

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
		to << L"L\"" << t.name.last() << L"\", ";

		// Package.
		if (t.pkg.empty())
			PLN(t.pos << L": warning: placing threads in the root package.");
		to << L"L\"" << t.pkg << L"\", ";

		// Declaration.
		to << L"&" << t.name << L"::decl";

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
		{ L"FN_PARAMETERS", &genFnParams },
		{ L"CPP_FUNCTIONS", &genFunctions },
		{ L"TEMPLATE_GLOBALS", &genTemplateGlobals },
		{ L"CPP_TEMPLATES", &genTemplates },
		{ L"THREAD_GLOBALS", &genThreadGlobals },
		{ L"CPP_THREADS", &genThreads },
	};

	GenerateMap g;
	for (nat i = 0; i < ARRAY_COUNT(e); i++)
		g.insert(make_pair(e[i].tag, e[i].fn));

	return g;
}
