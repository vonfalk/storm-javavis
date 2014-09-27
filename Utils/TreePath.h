#pragma once

#include "Printable.h"
#include <vector>

// This class represents a sequence of numbers, which in turns indicates
// the path to get to a specific node inside a (ordered) tree. Think of these
// numbers as filenames, then it works like regular file-system paths.
class TreePath : public Printable {
public:
	// Create an empty path.
	TreePath() {}

	// Create from iterator.
	template <class T>
	TreePath(T begin, T end) : data(begin, end) {}

	// Equality checks.
	bool operator ==(const TreePath &o) const;
	inline bool operator !=(const TreePath &o) const { return !(*this == o); }

	// Get/set elements.
	inline nat operator[](nat at) const { return data[at]; }
	inline nat &operator[](nat at) { return data[at]; }

	// Last and first element.
	nat front() const { return data.front(); }
	nat back() const { return data.back(); }

	// Get length.
	inline nat size() const { return data.size(); }

	// Are we a parent of "other"?
	bool isParent(const TreePath &other) const;

	// Get the parent path to this.
	TreePath parent() const;

	// Indicates an item's insertion point relative a path.
	enum Insert {
		before,
		after,
		child,
	};
protected:
	virtual void output(std::wostream &to) const;

private:
	vector<nat> data;
};
