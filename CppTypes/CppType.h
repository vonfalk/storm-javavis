#pragma once
#include "CppName.h"
#include "SrcPos.h"
#include "Auto.h"

/**
 * Represents a (written) type in C++.
 */
class CppType : public Refcount {
public:
	CppType(const SrcPos &pos);

	// Position of this type.
	SrcPos pos;

	// Is this type const?
	bool constType;

	// Print.
	virtual void print(wostream &to) const = 0;
};

inline wostream &operator <<(wostream &to, const CppType &c) {
	c.print(to);
	if (c.constType)
		to << L" const";
	return to;
}

/**
 * Array type.
 */
class ArrayType : public CppType {
public:
	ArrayType(Auto<CppType> of);

	// Array member type.
	Auto<CppType> of;

	// Print.
	virtual void print(wostream &to) const;
};

/**
 * Map type.
 */
class MapType : public CppType {
public:
	MapType(Auto<CppType> k, Auto<CppType> v);

	// Key and value types.
	Auto<CppType> k, v;

	// Print.
	virtual void print(wostream &to) const;
};

/**
 * Generic templated type. Only templates in the last position are supported at the moment.
 */
class TemplateType : public CppType {
public:
	TemplateType(const SrcPos &pos, const CppName &name);

	// Name.
	CppName name;

	// Template types.
	vector<Auto<CppType>> params;

	// Print.
	virtual void print(wostream &to) const;
};

/**
 * Pointer type.
 */
class PtrType : public CppType {
public:
	PtrType(Auto<CppType> of);

	// Type.
	Auto<CppType> of;

	// Print.
	virtual void print(wostream &to) const;
};

/**
 * Ref type.
 */
class RefType : public CppType {
public:
	RefType(Auto<CppType> of);

	// Type.
	Auto<CppType> of;

	// Print.
	virtual void print(wostream &to) const;
};

/**
 * Maybe type.
 */
class MaybeType : public CppType {
public:
	MaybeType(Auto<CppType> of);

	// Type.
	Auto<PtrType> of;

	// Print.
	virtual void print(wostream &to) const;
};

/**
 * Named type.
 */
class NamedType : public CppType {
public:
	NamedType(const SrcPos &pos, const CppName &name);
	NamedType(const SrcPos &pos, const String &name);

	// Name.
	CppName name;

	// Print.
	virtual void print(wostream &to) const;
};
