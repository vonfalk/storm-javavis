#include "stdafx.h"
#include "Utils/Path.h"
#include "Header.h"
#include "Function.h"
#include "Exception.h"
#include "Output.h"
#include "Write.h"

// #define DEBUG_MODE

static void findHeaders(vector<Header*> &output, const Path &p) {
	vector<Path> children = p.children();
	for (nat i = 0; i < children.size(); i++) {
		Path &c = children[i];

		if (c.isDir()) {
			findHeaders(output, c);
		} else if (c.hasExt(L"h")) {
			output.push_back(new Header(c));
		}
	}
}

bool compare(const Header *a, const Header *b) {
	return a->file < b->file;
}

vector<Header*> findHeaders(const vector<Path> &p) {
	vector<Header*> h;
	for (nat i = 0; i < p.size(); i++)
		findHeaders(h, p[i]);
	sort(h.begin(), h.end(), compare);
	return h;
}

Timestamp lastTime(const vector<Header *> &headers) {
	Timestamp latest = headers[0]->file.mTime();

	for (nat i = 1; i < headers.size(); i++) {
		limitMin(latest, headers[i]->file.mTime());
	}

	return latest;
}

Types allTypes(vector<Header *> &headers, bool forCompiler, const vector<String> &namespaces) {
	Types t(forCompiler);

	for (nat i = 0; i < namespaces.size(); i++) {
		t.usedNamespaces.push_back(CppName(namespaces[i].split(L"::")));
	}

	// built in types!
	t.add(Type(L"void", L""));
	t.add(Type(L"Int", L"core"));
	t.add(Type(L"Nat", L"core"));
	t.add(Type(L"Byte", L"core"));
	t.add(Type(L"Bool", L"core"));
	// t.add(Type(L"Type", L"core"));

	for (nat i = 0; i < headers.size(); i++) {
		Header &h = *headers[i];
		const vector<Type> &types = h.getTypes();

		for (nat i = 0; i < types.size(); i++) {
			t.add(types[i]);
		}
	}

	return t;
}

vector<Thread> allThreads(vector<Header *> &headers) {
	vector<Thread> threads;

	for (nat i = 0; i < headers.size(); i++) {
		Header &h = *headers[i];
		const vector<Thread> &t = h.getThreads();

		for (nat i = 0; i < t.size(); i++) {
			threads.push_back(t[i]);
		}
	}

	sort(threads.begin(), threads.end());

	return threads;
}

void usage(const String &msg) {
	using namespace std;
	wcout << L"Error: " << msg << endl;
	wcout << L"Options:" << endl;
	wcout << L"[--compiler]          - Generate table for the compiler itself (not a DLL)." << endl;
	wcout << L"[-a <asmOutput>]      - ASM output file." << endl;
	wcout << L"[--using <namespace>] - A global 'using namespace <namespace> is present somewhere." << endl;
	wcout << L"<root>                - Specify the root directory. All paths are relative this." << endl;
	wcout << L"<template input>      - Input template for the function list." << endl;
	wcout << L"<template output>     - Output file for the template." << endl;
	wcout << L"<input>               - (multiple) input directories to be scanned for header files." << endl;
}

int _tmain(int argc, _TCHAR* argv[]) {
	initDebug();

	Sleep(2000);

	Timestamp start;

	vector<String> namespaces;

#ifdef DEBUG_MODE
	// For debugging.
	Path root = Path::executable() + Path(L"../Storm/");
	vector<Path> scanDirs(1, root);

	Path input = root + Path(L"Lib/BuiltIn.template.cpp");
	Path output = root + Path(L"Lib/BuiltIn.cpp");
	Path asmOutput = root + Path(L"Lib/VTables.asm");

	bool forCompiler = true;
	bool forceUpdate = true;
#else

	bool forCompiler = false;
	// Inputs
	Path root, input;
	vector<Path> scanDirs;
	// Outputs
	Path output, asmOutput;

	if (argc < 4) {
		usage(L"Too few parameters.");
		return 1;
	}

	nat pos = 0;
	for (int i = 1; i < argc; i++) {
		String s = argv[i];
		if (s == L"--compiler") {
			forCompiler = true;
		} else if (s == L"--using") {
			namespaces.push_back(argv[++i]);
		} else if (s == L"-a") {
			asmOutput = Path(String(argv[++i]));
		} else if (pos == 0) {
			root = Path(s);
			pos++;
		} else if (pos == 1) {
			input = Path(s);
			pos++;
		} else if (pos == 2) {
			output = Path(s);
			pos++;
		} else if (pos == 3) {
			scanDirs.push_back(Path(s));
		}
	}

	bool forceUpdate = false;
#endif

	Timestamp outputModified = output.mTime();
	if (asmOutput.exists())
		limitMin(outputModified, asmOutput.mTime());

	vector<Header*> headers = findHeaders(scanDirs);
	if (headers.size() == 0) {
		usage(L"No input files!");
		return 1;
	}

	Timestamp inputModified = lastTime(headers);
	limitMin(inputModified, input.mTime());
	limitMin(inputModified, Path::executableFile().mTime());

	bool toDate = inputModified <= outputModified;
	toDate &= !forceUpdate;
	toDate &= output.exists();

	if (toDate) {
		std::wcout << L"Already up to date!" << std::endl;
	} else {
		try {
			Types t = allTypes(headers, forCompiler, namespaces);
			vector<Type> types = t.getTypes();
			vector<Thread> threads = allThreads(headers);
			FileData d;
			d.typeList = typeList(t, threads);
			d.typeFunctions = typeFunctions(t);
			d.vtableCode = vtableCode(t);
			d.functionList = functionList(headers, t, threads);
			d.variableList = variableList(headers, t);
			d.headerList = headerList(headers, root);
			d.threadList = threadList(threads);

			update(input, output, asmOutput, d);
		} catch (const Exception &e) {
			std::wcout << L"Error: " << e.what() << endl;
			clear(headers);
			return 2;
		}
	}

	Timestamp end;
	PLN("Total time: " << end - start);

	clear(headers);
	return 0;
}
