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

	// Clear.
	inline void clear() { parts.clear(); }

	// Concatenation.
	CppName operator +(const CppName &o) const;

	// Use in set.
	bool operator <(const CppName &o) const;

protected:
	virtual void output(wostream &to) const;

};

class Types;

class CppType : public Printable {
public:
	CppType() : isConst(false), isPtr(false), isRef(false) {}

	// name of the type.
	CppName type;

	// modifiers
	bool isConst, isPtr, isRef;

	// Clear.
	inline void clear() { type.clear(); isConst = isPtr = isRef = false; }

	// Read!
	static CppType read(Tokenizer &tok);

	// Void?
	bool isVoid() const;

	// Type* ?
	bool isTypePtr() const;

	static CppType typePtr();

	// Full name.
	CppType fullName(const Types &t, const CppName &scope) const;

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

	// Get the name of the current scope, ie cppName if !isType and cppName.parent() if isType
	CppName scopeName() const;

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
