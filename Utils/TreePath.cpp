#include "stdafx.h"
#include "TreePath.h"

bool TreePath::operator ==(const TreePath &other) const {
	if (size() != other.size()) return false;

	for (nat i = 0; i < size(); i++) {
		if (data[i] != other[i]) return false;
	}

	return true;
}

void TreePath::output(std::wostream &to) const {
	to << L"Path: " << join(data, L" -> ");
}

bool TreePath::isParent(const TreePath &other) const {
	if (size() >= other.size()) return false;

	for (nat i = 0; i < size(); i++) {
		if (data[i] != other[i]) return false;
	}

	return true;
}

TreePath TreePath::parent() const {
	return TreePath(data.begin(), data.end() - 1);
}