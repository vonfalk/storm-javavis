#include "stdafx.h"
#include "FileTypes.h"

FileTypes::FileTypes(const String &collName) : collName(collName) {}

void FileTypes::add(const String &ext, const String &name) {
	Elem e = { ext, name };
	types.push_back(e);
}

void FileTypes::addAll() {
	add(L"*", L"All files");
}

const FileTypes::Elem &FileTypes::operator [](nat id) const {
	return types[id];
}

vector<String> FileTypes::allTypes() const {
	vector<String> t;
	for (nat i = 0; i < types.size(); i++) {
		t.push_back(types[i].ext);
	}
	return t;
}
