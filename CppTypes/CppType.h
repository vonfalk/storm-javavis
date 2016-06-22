#pragma once
#include "CppName.h"
#include "SrcPos.h"
#include "Auto.h"

class Type;
class World;

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

	// Get the size of this type.
	virtual Size size() const = 0;

	// Is this a gc:d type?
	virtual bool gcType() const = 0;

	// Resolve type info.
	virtual Auto<CppType> resolve(World &in, const CppName &context) const = 0;

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

	// Get the size of this type.
	virtual Size size() const;

	// Is this a gc:d type?
	virtual bool gcType() const { return true; }

	// Resolve.
	virtual Auto<CppType> resolve(World &in, const CppName &context) const;

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

	// Get the size of this type.
	virtual Size size() const;

	// Is this a gc:d type?
	virtual bool gcType() const { return true; }

	// Resolve.
	virtual Auto<CppType> resolve(World &in, const CppName &context) const;

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

	// Get the size of this type.
	virtual Size size() const;

	// Is this a gc:d type?
	virtual bool gcType() const { return false; }

	// Resolve.
	virtual Auto<CppType> resolve(World &in, const CppName &context) const;

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

	// Get the size of this type.
	virtual Size size() const { return Size::sPtr; }

	// Is this a gc:d type?
	virtual bool gcType() const { return false; }

	// Resolve.
	virtual Auto<CppType> resolve(World &in, const CppName &context) const;

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

	// Get the size of this type.
	virtual Size size() const { return Size::sPtr; }

	// Is this a gc:d type?
	virtual bool gcType() const { return false; }

	// Resolve.
	virtual Auto<CppType> resolve(World &in, const CppName &context) const;

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

	// Get the size of this type.
	virtual Size size() const { return of->size(); }

	// Is this a gc:d type?
	virtual bool gcType() const { return of->gcType(); }

	// Resolve.
	virtual Auto<CppType> resolve(World &in, const CppName &context) const;

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

	// Get the size of this type.
	virtual Size size() const;

	// Is this a gc:d type?
	virtual bool gcType() const { return false; }

	// Resolve.
	virtual Auto<CppType> resolve(World &in, const CppName &context) const;

	// Print.
	virtual void print(wostream &to) const;
};

/**
 * Resolved type.
 */
class ResolvedType : public CppType {
public:
	ResolvedType(const CppType &templ, Type *type);

	// Type.
	Type *type;

	// Get the size of this type.
	virtual Size size() const;

	// Is this a gc:d type?
	virtual bool gcType() const;

	// Resolve.
	virtual Auto<CppType> resolve(World &in, const CppName &context) const;

	// Print.
	virtual void print(wostream &to) const;
};

/**
 * Type built into C++.
 */
class BuiltInType : public CppType {
public:
	BuiltInType(const SrcPos &pos, const String &name, Size size);

	// Name.
	String name;

	// Size.
	Size tSize;

	// Get the size of this type.
	virtual Size size() const { return tSize; }

	// Is this a gc:d type?
	virtual bool gcType() const { return false; }

	// Resolve.
	virtual Auto<CppType> resolve(World &in, const CppName &context) const;

	// Print.
	virtual void print(wostream &to) const;
};


inline bool isGcPtr(Auto<CppType> t) {
	if (Auto<PtrType> p = t.as<PtrType>()) {
		return p->of->gcType();
	} else if (Auto<RefType> r = t.as<RefType>()) {
		return r->of->gcType();
	} else {
		return false;
	}
}
