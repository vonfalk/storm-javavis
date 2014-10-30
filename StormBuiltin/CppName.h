#pragma once
#include "Tokenizer.h"

/**
 * Describes a name from C++.
 */
class CppName : public Printable {
public:
	CppName() {}
	CppName(vector<String> &parts) : parts(parts) {}

	static CppName read(Tokenizer &tok);

	// Parts to be joined by ::
	vector<String> parts;

	// Parent name.
	CppName parent() const;

	// Empty?
	inline bool empty() const { return parts.empty(); }

	// Concatenation.
	CppName operator +(const CppName &o) const;

	// Use in set.
	bool operator <(const CppName &o) const;

protected:
	virtual void output(wostream &to) const;

};

class CppScope : public Printable {
public:
	struct Part {
		// Is it a type?
		bool type;
		// Depth inside.
		nat depth;
		// Name of this part.
		String name;
		// Parent type.
		CppName super;
	};

	// Push a new name.
	void push(bool type, const String &name, const CppName &super = CppName());

	// Push a new subscope (unnamed).
	void push();

	// Pop one.
	void pop();

	// Get the current name.
	CppName cppName() const;

	// Is this a type?
	bool isType() const;

	// Super type of this type.
	CppName super() const;

	// Name of the topmost scope.
	String name() const;

protected:
	virtual void output(wostream &to) const;

private:
	vector<Part> parts;
};
