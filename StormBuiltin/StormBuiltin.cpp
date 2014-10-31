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

vector<Header*> findHeaders(const Path &p) {
	vector<Header*> h;
	findHeaders(h, p);
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

Types allTypes(vector<Header *> &headers) {
	Types t;

	// built in types!
	t.add(Type(L"void", L""));
	t.add(Type(L"Int", L"core"));
	t.add(Type(L"Nat", L"core"));
	t.add(Type(L"Type", L"core"));

	for (nat i = 0; i < headers.size(); i++) {
		Header &h = *headers[i];
		const vector<Type> &types = h.getTypes();

		for (nat i = 0; i < types.size(); i++) {
			t.add(types[i]);
		}
	}
	return t;
}

int _tmain(int argc, _TCHAR* argv[]) {
	initDebug();

	Timestamp start;

#ifdef DEBUG_MODE
	// For debugging.
	Path root = Path::executable() + Path(L"../Storm/");
	Path scanDir = root;

	Path output = root + Path(L"Lib/BuiltIn.cpp");
	Path asmOutput = root + Path(L"Lib/VTables.asm");

	bool forceUpdate = true;
#else

	// Inputs
	Path root, scanDir;
	// Outputs
	Path output, asmOutput;

	if (argc < 4) {
		std::wcout << L"Error: <root> <scanDir> <update> [<asmOutput>] required!" << std::endl;
		return 1;
	}

	root = Path(String(argv[1]));
	scanDir = Path(String(argv[2]));
	output = Path(String(argv[3]));

	bool forceUpdate = false;
#endif

	Timestamp outputModified = output.mTime();
	if (argc >= 5) {
		asmOutput = Path(String(argv[4]));
		limitMin(outputModified, asmOutput.mTime());
	}

	vector<Header*> headers = findHeaders(root);
	if (headers.size() == 0) {
		std::wcout << L"Error: No input files!" << endl;
		return 1;
	}

	Timestamp inputModified = lastTime(headers);

	if (inputModified <= outputModified && !forceUpdate) {
		std::wcout << L"Already up to date!" << std::endl;
	} else {
		try {
			Types t = allTypes(headers);
			FileData d;
			d.typeList = typeList(t);
			d.typeFunctions = typeFunctions(t);
			d.vtableCode = vtableCode(t);
			d.functionList = functionList(headers, t);
			d.headerList = headerList(headers, root);
			update(output, asmOutput, d);
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
