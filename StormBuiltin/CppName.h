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
	CppName operator +(const String &o) const;

	// Use in set.
	bool operator <(const CppName &o) const;
	inline bool operator ==(const CppName &o) const { return parts == o.parts; }
	inline bool operator !=(const CppName &o) const { return parts != o.parts; }

	// Is this the root Object class?
	bool isObject() const;

protected:
	virtual void output(wostream &to) const;

};

class Types;

class CppType : public Printable {
public:
	CppType() :
		isConst(false), isPtr(false), isRef(false),
		isAuto(false), isPar(false), isArray(false),
		isArrayP(false), isFnPtr(false) {}

	// name of the type. In the case of a function pointer, this is not relevant.
	CppName type;

	// Parameters to the function call. The first is the return value.
	vector<CppType> fnParams;

	// modifiers
	bool isConst, isPtr, isRef, isAuto, isPar, isArray, isArrayP, isFnPtr;

	// Clear.
	inline void clear() { type.clear(); isConst = isPtr = isRef = isAuto = isArray = isArrayP = isFnPtr = false; }

	// Read!
	static CppType read(Tokenizer &tok);

	// Empty?
	inline bool isEmpty() const { return type.empty(); }

	// Void?
	bool isVoid() const;

	// Void type.
	static CppType tVoid();

	// Type* ?
	bool isTypePtr() const;

	static CppType typePtr();

	// Full name.
	CppType fullName(const Types &t, const CppName &scope) const;

protected:
	virtual void output(wostream &to) const;
};

class CppSuper : public Printable {
public:
	inline CppSuper() : isHidden(false), isThread(false) {}

	// Name of the super type.
	CppName name;

	// Is the super type hidden?
	bool isHidden;

	// Actually a thread?
	bool isThread;

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
		CppSuper super;
	};

	// Push a new name.
	void push(bool type, const String &name, const CppSuper &super = CppSuper());

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
	CppSuper super() const;

	// Name of the topmost scope.
	String name() const;

protected:
	virtual void output(wostream &to) const;

private:
	vector<Part> parts;

	// Depth in the root block.
	nat rootDepth;
};
