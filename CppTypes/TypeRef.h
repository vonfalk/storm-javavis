#pragma once
#include "CppName.h"
#include "SrcPos.h"
#include "Auto.h"

class Type;
class World;

/**
 * Represents a (written) type in C++.
 */
class TypeRef : public Refcount {
public:
	TypeRef(const SrcPos &pos);

	// Position of this type.
	SrcPos pos;

	// Is this type const?
	bool constType;

	// Get the size of this type.
	virtual Size size() const = 0;

	// Is this a gc:d type?
	virtual bool gcType() const = 0;

	// Resolve type info.
	virtual Auto<TypeRef> resolve(World &in, const CppName &context) const = 0;

	// Print.
	virtual void print(wostream &to) const = 0;
};

inline wostream &operator <<(wostream &to, const TypeRef &c) {
	c.print(to);
	if (c.constType)
		to << L" const";
	return to;
}

/**
 * Array type.
 */
class ArrayType : public TypeRef {
public:
	ArrayType(Auto<TypeRef> of);

	// Array member type.
	Auto<TypeRef> of;

	// Get the size of this type.
	virtual Size size() const;

	// Is this a gc:d type?
	virtual bool gcType() const { return true; }

	// Resolve.
	virtual Auto<TypeRef> resolve(World &in, const CppName &context) const;

	// Print.
	virtual void print(wostream &to) const;
};

/**
 * Map type.
 */
class MapType : public TypeRef {
public:
	MapType(Auto<TypeRef> k, Auto<TypeRef> v);

	// Key and value types.
	Auto<TypeRef> k, v;

	// Get the size of this type.
	virtual Size size() const;

	// Is this a gc:d type?
	virtual bool gcType() const { return true; }

	// Resolve.
	virtual Auto<TypeRef> resolve(World &in, const CppName &context) const;

	// Print.
	virtual void print(wostream &to) const;
};

/**
 * Generic templated type. Only templates in the last position are supported at the moment.
 */
class TemplateType : public TypeRef {
public:
	TemplateType(const SrcPos &pos, const CppName &name);

	// Name.
	CppName name;

	// Template types.
	vector<Auto<TypeRef>> params;

	// Get the size of this type.
	virtual Size size() const;

	// Is this a gc:d type?
	virtual bool gcType() const { return false; }

	// Resolve.
	virtual Auto<TypeRef> resolve(World &in, const CppName &context) const;

	// Print.
	virtual void print(wostream &to) const;
};

/**
 * Pointer type.
 */
class PtrType : public TypeRef {
public:
	PtrType(Auto<TypeRef> of);

	// Type.
	Auto<TypeRef> of;

	// Get the size of this type.
	virtual Size size() const { return Size::sPtr; }

	// Is this a gc:d type?
	virtual bool gcType() const { return false; }

	// Resolve.
	virtual Auto<TypeRef> resolve(World &in, const CppName &context) const;

	// Print.
	virtual void print(wostream &to) const;
};

/**
 * Ref type.
 */
class RefType : public TypeRef {
public:
	RefType(Auto<TypeRef> of);

	// Type.
	Auto<TypeRef> of;

	// Get the size of this type.
	virtual Size size() const { return Size::sPtr; }

	// Is this a gc:d type?
	virtual bool gcType() const { return false; }

	// Resolve.
	virtual Auto<TypeRef> resolve(World &in, const CppName &context) const;

	// Print.
	virtual void print(wostream &to) const;
};

/**
 * Maybe type.
 */
class MaybeType : public TypeRef {
public:
	MaybeType(Auto<TypeRef> of);

	// Type.
	Auto<PtrType> of;

	// Get the size of this type.
	virtual Size size() const { return of->size(); }

	// Is this a gc:d type?
	virtual bool gcType() const { return of->gcType(); }

	// Resolve.
	virtual Auto<TypeRef> resolve(World &in, const CppName &context) const;

	// Print.
	virtual void print(wostream &to) const;
};

/**
 * Named type.
 */
class NamedType : public TypeRef {
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
	virtual Auto<TypeRef> resolve(World &in, const CppName &context) const;

	// Print.
	virtual void print(wostream &to) const;
};

/**
 * Resolved type.
 */
class ResolvedType : public TypeRef {
public:
	ResolvedType(const TypeRef &templ, Type *type);

	// Type.
	Type *type;

	// Get the size of this type.
	virtual Size size() const;

	// Is this a gc:d type?
	virtual bool gcType() const;

	// Resolve.
	virtual Auto<TypeRef> resolve(World &in, const CppName &context) const;

	// Print.
	virtual void print(wostream &to) const;
};

/**
 * Type built into C++.
 */
class BuiltInType : public TypeRef {
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
	virtual Auto<TypeRef> resolve(World &in, const CppName &context) const;

	// Print.
	virtual void print(wostream &to) const;
};

/**
 * GcArray-type.
 */
class GcArrayType : public TypeRef {
public:
	GcArrayType(const SrcPos &pos, Auto<TypeRef> of);

	// Type of what?
	Auto<TypeRef> of;

	// Get the size of this type.
	virtual Size size() const;

	// Is this a gc:d type?
	virtual bool gcType() const { return true; }

	// Resolve.
	virtual Auto<TypeRef> resolve(World &in, const CppName &context) const;

	// Print.
	virtual void print(wostream &to) const;
};


inline bool isGcPtr(Auto<TypeRef> t) {
	if (Auto<PtrType> p = t.as<PtrType>()) {
		return p->of->gcType();
	} else if (Auto<RefType> r = t.as<RefType>()) {
		return r->of->gcType();
	} else {
		return false;
	}
}
