#include "stdafx.h"
#include "Utils/Path.h"
#include "Function.h"
#include "Exception.h"
#include "Utils/FileStream.h"
#include "Utils/TextReader.h"

using namespace stormbuiltin;
using namespace util;

void findFunctions(const Path &p, File &result) {
	vector<Path> children = p.children();
	for (nat i = 0; i < children.size(); i++) {
		Path &c = children[i];

		if (c.isDir()) {
			findFunctions(c, result);
		} else if (c.hasExt(L"h")) {
			File f = parseFile(c);
			result.add(f);
		}
	}
}

bool compareFn(const Function &a, const Function &b) {
	if (a.package != b.package)
		return a.package < b.package;
	if (a.name != b.name)
		return a.name < b.name;
	return a.params < b.params;
}

bool compareType(const Type &a, const Type &b) {
	if (a.package != b.package)
		return a.package < b.package;
	return a.name < b.name;
}

String fixPath(String str) {
	for (nat i = 0; i < str.size(); i++)
		if (str[i] == '\\')
			str[i] = '/';
	return str;
}

String indentation(const String &line) {
	for (nat i = 0; i < line.size(); i++) {
		switch (line[i]) {
		case ' ':
		case '\t':
			break;
		default:
			return line.substr(0, i);
		}
	}
	return line;
}

void addLines(vector<String> &to, const String &from, const String &prepend) {
	vector<String> l = from.split(L"\n");
	for (nat i = 0; i < l.size(); i++)
		to.push_back(prepend + l[i]);
}

String lineEnding(const Path &file) {
	TextReader *r = TextReader::create(new FileStream(file, Stream::mRead));
	String result = L"\n";

	wchar prev = 0;
	while (r->more()) {
		wchar c = r->get();
		if (prev == '\r' && c == '\n') {
			result = L"\r\n";
			break;
		} else if (c == '\n') {
			result = L"\n";
			break;
		}
		prev = c;
	}

	delete r;

	return result;
}

void updateFile(const Path &file,
				const Path &asmFile,
				const String &headers,
				const String &fnList,
				const String &typeList,
				const String &typeDecls,
				const String &asmContent) {

	vector<String> lines;
	String newline = lineEnding(file);

	TextReader *r = TextReader::create(new FileStream(file, Stream::mRead));
	bool keep = true;

	while (r->more()) {
		String line = r->getLine();
		if (line.find(L"// BEGIN LIST") != String::npos) {
			keep = false;
			lines.push_back(line);
			addLines(lines, fnList, indentation(line));
		} else if (line.find(L"// END LIST") != String::npos) {
			keep = true;
			lines.push_back(line);
		} else if (line.find(L"// BEGIN INCLUDES") != String::npos) {
			keep = false;
			lines.push_back(line);
			addLines(lines, headers, indentation(line));
		} else if (line.find(L"// END INCLUDES") != String::npos) {
			keep = true;
			lines.push_back(line);
		} else if (line.find(L"// BEGIN TYPES") != String::npos) {
			keep = false;
			lines.push_back(line);
			addLines(lines, typeList, indentation(line));
		} else if (line.find(L"// END TYPES") != String::npos) {
			keep = true;
			lines.push_back(line);
		} else if (line.find(L"// BEGIN STATIC") != String::npos) {
			keep = false;
			lines.push_back(line);
			addLines(lines, typeDecls, indentation(line));
		} else if (line.find(L"// END STATIC") != String::npos) {
			keep = true;
			lines.push_back(line);
		} else if (keep) {
			lines.push_back(line);
		}
	}

	textfile::Format fmt = r->format();
	delete r;

	TextWriter *w = TextWriter::create(new FileStream(file, Stream::mWrite), fmt);

	for (nat i = 0; i < lines.size(); i++) {
		if (i > 0)
			w->put(newline);
		w->put(lines[i]);
	}
	delete w;

	w = TextWriter::create(new FileStream(asmFile, Stream::mWrite), fmt);
	lines.clear();
	addLines(lines, asmContent, L"");
	for (nat i = 0; i < lines.size(); i++) {
		if (i > 0)
			w->put(newline);
		w->put(lines[i]);
	}
	delete w;

}

String vtableSymbol(const Type &t) {
	vector<String> parts = t.cppName.split(L"::");
	std::wostringstream r;
	r << L"??_7";
	for (nat i = parts.size(); i > 0; i--) {
		r << parts[i-1] << L"@";
	}
	r << L"@6B@";
	return r.str();
}

String vtableFnName(const Type &t) {
	String cppName = t.cppName;
	for (nat i = 0; i < cppName.size(); i++)
		if (cppName[i] == ':')
			cppName[i] = '_';
	return L"cppVTable_" + cppName;
}

String asmContent(const vector<Type> &types) {
	std::wostringstream c;
	c << L".386\n";
	c << L".model flat, c\n\n";
	c << L".data\n\n";

	for (nat i = 0; i < types.size(); i++) {
		const Type &t = types[i];
		String sym = vtableSymbol(t);
		String fn = vtableFnName(t);

		c << sym << L" proto syscall\n";
		c << fn << L" proto\n\n";
	}

	c << L"\n.code\n\n";

	for (nat i = 0; i < types.size(); i++) {
		const Type &t = types[i];
		String sym = vtableSymbol(t);
		String fn = vtableFnName(t);

		c << fn << L" proc\n";
		c << L"\tmov eax, " << sym << L"\n";
		c << L"\tret\n";
		c << fn << L" endp\n\n";
	}
	c << L"end\n";

	return c.str();
}

void generateBuiltin(const Path &root, const Path &headerRoot, const Path &listFile, const Path &asmFile) {
	File result;
	findFunctions(headerRoot, result);

	set<String> headers;
	for (nat i = 0; i < result.fns.size(); i++) {
		Path r = result.fns[i].header.makeRelative(root);
		headers.insert(r.toS());
	}

	std::wostringstream headerStr;
	for (set<String>::iterator i = headers.begin(); i != headers.end(); i++) {
		headerStr << L"#include \"" << fixPath(*i) << "\"\n";
	}

	sort(result.fns.begin(), result.fns.end(), compareFn);
	std::wostringstream codeStr;
	for (nat i = 0; i < result.fns.size(); i++) {
		Function &f = result.fns[i];

		codeStr << L"{ Name(L\"" << f.package << L"\"), ";
		if (f.classMember.empty())
			codeStr << L"null";
		else
			codeStr << 'L' << '"' << f.classMember << '"';
		codeStr << L", ";
		if (f.result == L"void")
			codeStr << L"Name(), ";
		else
			codeStr << L"Name(L\"" << f.result << L"\"), ";
		codeStr << L"L\"" << f.name << L"\", ";
		codeStr << L"list(" << f.params.size();
		for (nat j = 0; j < f.params.size(); j++) {
			codeStr << L", Name(L\"" << f.params[j] << L"\")";
		}
		codeStr << L"), address(&" << f.cppName << L") },\n";
	}

	sort(result.types.begin(), result.types.end(), compareType);
	std::wostringstream typeStr;
	for (nat i = 0; i < result.types.size(); i++) {
		Type &t = result.types[i];

		typeStr << L"{ Name(L\"" << t.package << L"\"), ";
		typeStr << L"L\"" << t.name << L"\", ";
		if (t.super.empty())
			typeStr << L"null";
		else
			typeStr << L"L\"" << t.super << L"\"";
		typeStr << L", " << i;
		typeStr << " }," << endl;
	}

	std::wostringstream declStr;
	for (nat i = 0; i < result.types.size(); i++) {
		Type &t = result.types[i];
		declStr << L"storm::Type *" << t.cppName << L"::type(Engine &e) { return e.builtIn(" << i << L"); }\n";
		declStr << L"storm::Type *" << t.cppName << L"::type(Object *o) { return type(o->myType->engine); }\n";
		String fnName = vtableFnName(t);
		declStr << L"extern \"C\" void *" << fnName << L"();\n";
		declStr << L"void *" << t.cppName << L"::cppVTable() { return " << fnName << L"(); }\n";
	}

	updateFile(listFile,
			asmFile,
			headerStr.str(),
			codeStr.str(),
			typeStr.str(),
			declStr.str(),
			asmContent(result.types));
}

int _tmain(int argc, _TCHAR* argv[]) {
	initDebug();

	TODO("Do not alter the outputs if no changes are made!");

	Path root, scanDir, output, asmOutput;
	if (argc < 5) {
		std::wcout << L"Error: <root> <scanDir> <output> <asmOutput> must be provided!" << std::endl;
		return 1;
	}

	root = Path(String(argv[1]));
	scanDir = Path(String(argv[2]));
	output = Path(String(argv[3]));
	asmOutput = Path(String(argv[4]));

	try {
		generateBuiltin(root, scanDir, output, asmOutput);
	} catch (const Exception &e) {
		std::wcout << L"Error: " << e.what() << std::endl;
		return 2;
	}

	return 0;
}

