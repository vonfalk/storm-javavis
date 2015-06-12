#include "stdafx.h"
#include "Output.h"
#include "Exception.h"

/**
 * Helpers
 */

static bool ignoreVTable(const Type &t) {
	return t.external && t.name != L"Object";
}

void vtableSymbolName(wostream &to, const CppName &name) {
	to << L"??_7";
	for (nat i = name.parts.size(); i > 0; i--) {
		to << name.parts[i-1] << L"@";
	}
	to << L"@6B@";
}

String vtableFnName(const CppName &name) {
	String cppName = toS(name);
	for (nat i = 0; i < cppName.size(); i++)
		if (cppName[i] == ':')
			cppName[i] = '_';
	return L"cppVTable_" + cppName;
}

void fnPtr(wostream &to, const Function &fn, const Types &types) {
	CppName scope = fn.cppScope.scopeName();

	to << fn.result.fullName(types, scope) << "(CODECALL ";
	if (fn.cppScope.isType())
		to << fn.cppScope.cppName() << L"::";

	to << L"*)(";
	for (nat i = 0; i < fn.params.size(); i++) {
		if (i != 0)
			to << L", ";
		if ((fn.flags & fnEngine) != 0 && i == 0)
			to << L"storm::EnginePtr";
		else
			to << fn.params[i].fullName(types, scope);
	}
	to << L")";
	if (fn.flags & fnConst)
		to << L" const";
}

String fixPath(String str) {
	for (nat i = 0; i < str.size(); i++)
		if (str[i] == '\\')
			str[i] = '/';
	return str;
}

/**
 * Find the id of a thread via CppName.
 */
nat threadId(const vector<Thread> &threads, const CppName &ref, const CppName &scope) {
	for (nat i = 0; i < threads.size(); i++) {
		if (threads[i].cppName == ref)
			return i;
		if (threads[i].cppName == scope + ref)
			return i;
	}

	return 0;
}

/**
 * Generation
 */

String typeList(const Types &types, const vector<Thread> &threads) {
	std::wostringstream out;
	const vector<Type> &t = types.getTypes();
	for (nat i = 0; i < t.size(); i++) {
		const Type &type = t[i];

		out << L"{ L\"" << type.package << L"\", ";
		out << L"L\"" << type.name << L"\", ";

		out << i << L" /* id */, ";

		const CppSuper &super = type.super;
		bool external = type.external;
		if (external) {
			out << 0 << L" /* ignored */, ";
			out << L"BuiltInType::superExternal, ";
		} else if (super.isThread) {
			nat threadId = ::threadId(threads, super.name, type.cppName.parent());
			out << threadId << L" /* " << super.name << L" */, ";
			out << L"BuiltInType::superThread, ";
		} else if (super.name.empty()) {
			out << 0 << L" /* none */, ";
			out << L"BuiltInType::superNone, ";
		} else {
			Type st = types.find(super.name, type.cppName.parent());
			out << types.typeId(st.cppName) << L" /* " << st.fullName() << " */, ";

			if (super.isHidden)
				out << L"BuiltInType::superHidden, ";
			else
				out << L"BuiltInType::superClass, ";
		}

		out << L"sizeof(" << type.cppName << L"), ";

		if (type.value) {
			out << L"typeValue, ";
		} else {
			out << L"typeClass, ";
		}

		if (type.value || external) {
			out << L"null ";
		} else {
			out << type.cppName << L"::cppVTable() ";
		}
		out << L"},\n";
	}

	return out.str();
}

String typeFunctions(const Types &types) {
	std::wostringstream out;
	const vector<Type> &t = types.getTypes();
	for (nat i = 0; i < t.size(); i++) {
		const Type &type = t[i];

		String fn = vtableFnName(type.cppName);

		out << L"storm::Type *" << type.cppName << L"::stormType(Engine &e) { return e.builtIn(" << i << L"); }\n";
		if (type.value) {
			// Values do not have type information.
		} else {
			if (!ignoreVTable(type)) {
				out << L"extern \"C\" void *" << fn << L"();\n";
			}

			if (type.external) {
				// Forward the vtable request to the main Engine for external types.
				out << L"void *" << type.cppName << L"::cppVTable() { return storm::cppVTable(" << i << L"); }\n";
			} else {
				out << L"void *" << type.cppName << L"::cppVTable() { return " << fn << L"(); }\n";
			}
		}
	}

	return out.str();
}

String vtableCode(const Types &types) {
	std::wostringstream out;
	const vector<Type> &t = types.getTypes();

	out << L".386\n";
	out << L".model flat, c\n\n";
	out << L".data\n\n";

	for (nat i = 0; i < t.size(); i++) {
		const Type &type = t[i];
		if (type.value || ignoreVTable(type))
			continue;

		vtableSymbolName(out, type.cppName);
		out << L" proto syscall\n";
		out << vtableFnName(type.cppName) << L" proto\n\n";
	}

	out << L"\n.code\n\n";

	for (nat i = 0; i < t.size(); i++) {
		const Type &type = t[i];
		if (type.value || ignoreVTable(type))
			continue;

		String fn = vtableFnName(type.cppName);

		out << fn << L" proc\n";
		out << L"\tmov eax, "; vtableSymbolName(out, type.cppName); out << L"\n";
		out << L"\tret\n";
		out << fn << L" endp\n\n";
	}

	out << L"end\n";

	return out.str();
}

static String valueRef(const CppType &type, const CppName &scope, const Types &types) {
	std::wostringstream out;

	if (type.isMaybe)
		out << L"maybeRef(";

	if (type.isArray || type.isArrayP)
		out << L"arrayRef(";

	if (type.isFnPtr) {
		out << L"ptrRef(";
		for (nat i = 0; i < type.fnParams.size(); i++) {
			if (i != 0)
				out << L", ";
			out << valueRef(type.fnParams[i], scope, types);
		}
		out << L")";
	} else {
		out << L"valueRef(";

		if (!type.isVoid()) {
			Type full = types.find(type.type, scope);
			out << L"L\"";
			out << full.fullName();
			out << L"\", ";
			if (full.value && (type.isRef || type.isPtr))
				out << L"true";
			else
				out << L"false";
		}
		out << L")";
	}

	if (type.isArray || type.isArrayP)
		out << L")";

	if (type.isMaybe)
		out << L")";

	return out.str();
}

static String stormName(const String &cppName) {
	if (cppName.left(9) == L"operator ")
		return cppName.substr(9);
	else
		return cppName;
}

void functionList(wostream &out, const vector<Function> &fns, const Types &types, const vector<Thread> &threads) {
	for (nat i = 0; i < fns.size(); i++) {
		const Function &fn = fns[i];
		CppName scope = fn.cppScope.scopeName();
		bool member = fn.cppScope.isType();

		if (fn.flags & fnExternal)
			continue;

		out << L"{ ";

		// Flags.
		if (member)
			out << L"BuiltInFunction::typeMember";
		else
			out << L"BuiltInFunction::noMember";

		if (!fn.thread.empty())
			out << L" | BuiltInFunction::onThread";
		if (fn.flags & fnEngine)
			out << L" | BuiltInFunction::hiddenEngine";
		if (fn.flags & fnVirtual)
			out << L" | BuiltInFunction::virtualFunction";
		if (fn.flags & fnSetter)
			out << L" | BuiltInFunction::setterFunction";
		out << L", ";

		// Member of?
		if (member) {
			Type memberOf = types.find(fn.cppScope.cppName(), scope);
			out << L"null, ";
			out << types.typeId(memberOf.cppName) << L" /* " << memberOf.fullName() << L" */, ";
		} else {
			out << "L\"" << fn.package << L"\", 0 /* -invalid- */, ";
		}

		// Result
		out << valueRef(fn.result, scope, types) << L", ";

		// Name
		out << L"L\"" << stormName(fn.name) << L"\", ";

		CppName name = fn.cppScope.cppName() + CppName(vector<String>(1, fn.name));

		// Params
		vector<CppType> params = fn.params;
		if (fn.flags & fnEngine) {
			// Ignore the first one!
			if (params.size() == 0)
				throw Error(toS(name) + L" is marked with STORM_ENGINE_FN and must take an Engine as the first parameter.");
			params.erase(params.begin());
		}

		out << L"list(" << params.size();
		for (nat i = 0; i < params.size(); i++) {
			out << L", " << valueRef(params[i], scope, types);
		}
		out << L"), ";

		// Function ptr.
		if (fn.name == L"__ctor") {
			out << L"address(&create" << (fn.params.size() + 1) << L"<";
			out << fn.cppScope.cppName();
			for (nat i = 0; i < fn.params.size(); i++) {
				CppType t = fn.params[i].fullName(types, scope);
				if (t.isPar) {
					// Efficiency hack: pass by ptr to the create-fn,
					// it will be casted to Auto by C++ later and do
					// the right thing in all cases anyway.
					t.isPar = false;
					t.isPtr = true;
				}
				out << L", " << t;
			}
			out << L">), ";
		} else if (fn.name == L"__dtor") {
			out << L"address(&destroy<" << fn.cppScope.cppName() << L">), ";
		} else if (fn.name == L"operator =") {
			out << L"address(&wrapAssign<" << fn.cppScope.cppName() << L">), ";
		} else {
			out << L"address<";
			fnPtr(out, fn, types);
			out << L">(&" << name << L"), ";
		}

		// Thread id (if needed).
		if (fn.thread.empty()) {
			out << L"0 /* no thread */";
		} else {
			nat threadId = ::threadId(threads, fn.thread, fn.cppScope.cppName());
			out << threadId << L" /* " << fn.thread << L" */";
		}

		out << " },\n";
	}
}

String functionList(const vector<Header *> &headers, const Types &types, const vector<Thread> &threads) {
	std::wostringstream out;

	for (nat i = 0; i < headers.size(); i++) {
		Header &header = *headers[i];
		const vector<Function> &fns = header.getFunctions();

		try {
			functionList(out, fns, types, threads);
		} catch (const Error &e) {
			Error err(e.what(), header.file);
			throw err;
		}
	}

	return out.str();
}

void variableList(std::wostream &to, const vector<Variable> &vars, const Types &types) {
	for (nat i = 0; i < vars.size(); i++) {
		const Variable &v = vars[i];

		if (v.external)
			continue;

		to << L"{ ";

		// Member of.
		CppName scope = v.cppScope.cppName();
		to << types.typeId(scope) << L" /* " << scope << L" */, ";

		// Type.
		to << valueRef(v.type, scope, types) << L", ";

		// Name.
		to << L"L\"" << v.name << L"\", ";

		// Offset.
		to << L"OFFSET_OF(" << scope << L", " << v.name << L")";

		to << L" }," << endl;
	}
}

String variableList(const vector<Header *> &headers, const Types &types) {
	std::wostringstream out;

	for (nat i = 0; i < headers.size(); i++) {
		Header &header = *headers[i];
		const vector<Variable> &vars = header.getVariables();

		try {
			variableList(out, vars, types);
		} catch (const Error &e) {
			Error err(e.what(), header.file);
			throw err;
		}
	}

	return out.str();
}

String headerList(const vector<Header *> &headers, const Path &root) {
	std::wostringstream out;

	for (nat i = 0; i < headers.size(); i++) {
		Header &header = *headers[i];

		if (header.getFunctions().size() > 0 || header.getTypes().size() > 0 || header.getThreads().size() > 0) {
			Path rel = header.file.makeRelative(root);
			out << L"#include \"" << fixPath(rel.toS()) << L"\"\n";
		}
	}

	return out.str();
}

String threadList(const vector<Thread> &threads) {
	std::wostringstream out;

	for (nat i = 0; i < threads.size(); i++) {
		const Thread &t = threads[i];
		if (t.external)
			continue;

		out << L"{ L\"" << t.pkg << L"\", ";
		out << L"L\"" << t.name << L"\", ";
		out << L"&" << t.cppName << "::decl },\n";
	}

	return out.str();
}
