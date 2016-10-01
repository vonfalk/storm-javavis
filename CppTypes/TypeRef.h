#pragma once
#include "CppName.h"
#include "SrcPos.h"
#include "Auto.h"
#include "Size.h"

class Type;
class Template;
class World;

/**
 * Represents a reference to a type in C++. (ie. whenever we're using a type).
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
 * Generic templated type. Only templates in the last position are supported at the moment.
 */
class TemplateType : public TypeRef {
public:
	TemplateType(const SrcPos &pos, const CppName &name, const vector<Auto<TypeRef>> &params = vector<Auto<TypeRef>>());

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
 * Resolved templated type.
 */
class ResolvedTemplateType : public TypeRef {
public:
	ResolvedTemplateType(const SrcPos &pos, Template *templ, const vector<Auto<TypeRef>> &params);

	// Template type.
	Template *type;

	// Template types.
	vector<Auto<TypeRef>> params;

	// Get size of this type.
	virtual Size size() const;

	// Is this a gc:d type?
	virtual bool gcType() const { return true; }

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
	explicit ResolvedType(Type *type);
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
 * GcArray type.
 */
class GcArrayType : public TypeRef {
public:
	GcArrayType(const SrcPos &pos, Auto<TypeRef> of, bool weak);

	// Type of what?
	Auto<TypeRef> of;

	// Weak?
	bool weak;

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
 * GcWatch type.
 */
class GcWatchType : public TypeRef {
public:
	GcWatchType(const SrcPos &pos);

	// Size of this type.
	virtual Size size() const;

	// Gc:d type?
	virtual bool gcType() const { return true; }

	// Resolve.
	virtual Auto<TypeRef> resolve(World &in, const CppName &context) const;

	// Print.
	virtual void print(wostream &to) const;
};

/**
 * EnginePtr type.
 */
class EnginePtrType : public TypeRef {
public:
	EnginePtrType(const SrcPos &pos) : TypeRef(pos) {}

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
 * Unknown type. These are to allow GC:d classes to contain types unknown to the preprocessor
 * without having to allocate them separatly all the time.
 *
 * Supports various kind of external types:
 * PTR_NOGC - pointer to non-gc object.
 * PTR_GC - pointer to gc object.
 * INT - integer sized object.
 * LONG - long-sized object (ie. 64 bits according to Storm terminology).
 */
class UnknownType : public TypeRef {
public:
	// Create. 'id' is the kind of external type.
	UnknownType(const String &id, Auto<TypeRef> of);

	// Type of what?
	Auto<TypeRef> of;

	// Get the size of this type.
	virtual Size size() const;

	// Is this a gc:d type?
	virtual bool gcType() const;

	// Resolve.
	virtual Auto<TypeRef> resolve(World &in, const CppName &context) const;

	// Print.
	virtual void print(wostream &to) const;

private:
	// Description of an id.
	struct ID {
		const wchar *name;
		const Size &size; // needs to be a reference for some reason.
		bool gc;
	};

	// All known ids.
	static const ID ids[];

	// Current id.
	const ID *id;
};


inline bool isGcPtr(Auto<TypeRef> t) {
	if (Auto<PtrType> p = t.as<PtrType>()) {
		return p->of->gcType();
	} else if (Auto<RefType> r = t.as<RefType>()) {
		return r->of->gcType();
	} else if (Auto<MaybeType> r = t.as<MaybeType>()) {
		return isGcPtr(r->of);
	} else if (Auto<UnknownType> r = t.as<UnknownType>()) {
		return r->gcType();
	} else {
		return false;
	}
}
