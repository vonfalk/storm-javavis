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

void updateFile(const Path &file, const String &headers, const String &fnList, const String &typeList) {
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
}

void generateBuiltin(const Path &root, const Path &headerRoot, const Path &listFile) {
	File result;
	findFunctions(headerRoot, result);

	set<String> headers;
	for (nat i = 0; i < result.fns.size(); i++) {
		Path r = result.fns[i].header.makeRelative(root);
		headers.insert(r.toS());
	}

	std::wostringstream headerStr;
	for (set<String>::iterator i = headers.begin(); i != headers.end(); i++) {
		headerStr << L"#include \"" << fixPath(*i) << "\"";
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
		typeStr << L"L\"" << t.name << L"\" }," << endl;
		PLN("Found type:");
	}

	updateFile(listFile, headerStr.str(), codeStr.str(), typeStr.str());
}

int _tmain(int argc, _TCHAR* argv[]) {
	initDebug();

	Path root, scanDir, output;
	if (argc < 4) {
		std::wcout << L"Error: <root> <scanDir> <output> must be provided!" << std::endl;
		return 1;
	}

	root = Path(String(argv[1]));
	scanDir = Path(String(argv[2]));
	output = Path(String(argv[3]));

	try {
		generateBuiltin(root, scanDir, output);
	} catch (const Exception &e) {
		std::wcout << L"Error: " << e.what() << std::endl;
		return 2;
	}

	return 0;
}

