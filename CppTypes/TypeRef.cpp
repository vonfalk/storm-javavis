#include "stdafx.h"
#include "TypeRef.h"
#include "Exception.h"
#include "World.h"

TypeRef::TypeRef(const SrcPos &pos) : pos(pos), constType(false) {}

ArrayType::ArrayType(Auto<TypeRef> of) : TypeRef(of->pos), of(of) {}

Size ArrayType::size() const {
	throw Error(L"Array<> should only be used as a pointer!", pos);
}

Auto<TypeRef> ArrayType::resolve(World &in, const CppName &context) const {
	Auto<ArrayType> r = new ArrayType(*this);
	r->of = of->resolve(in, context);
	return r;
}

void ArrayType::print(wostream &to) const {
	to << L"storm::Array<" << of << L">";
}

MapType::MapType(Auto<TypeRef> k, Auto<TypeRef> v) : TypeRef(k->pos), k(k), v(v) {}

Size MapType::size() const {
	throw Error(L"Map<> should only be used as a pointer!", pos);
}

Auto<TypeRef> MapType::resolve(World &in, const CppName &context) const {
	Auto<MapType> r = new MapType(*this);
	r->k = k->resolve(in, context);
	r->v = k->resolve(in, context);
	return r;
}

void MapType::print(wostream &to) const {
	to << L"storm::Map<" << k << L", " << v << L">";
}

TemplateType::TemplateType(const SrcPos &pos, const CppName &name) : TypeRef(pos), name(name) {}

Size TemplateType::size() const {
	throw Error(L"Templates are unsupported in general!", pos);
}

Auto<TypeRef> TemplateType::resolve(World &in, const CppName &context) const {
	if (name == L"Array" && params.size() == 1) {
		return new ArrayType(params[0]->resolve(in, context));
	} else if (name == L"Map" && params.size() == 2) {
		return new MapType(params[0]->resolve(in, context), params[1]->resolve(in, context));
	} else if (name == L"GcArray" && params.size() == 1) {
		return new GcArrayType(pos, params[0]->resolve(in, context));
	} else if (name == L"GcDynArray" && params.size() == 1) {
		return new GcDynArrayType(pos, params[0]->resolve(in, context));
	} else {
		throw Error(L"Unknown template type: " + toS(this), pos);
	}
}

void TemplateType::print(wostream &to) const {
	to << name << L"<";
	join(to, params, L", ");
	to << L">";
}

PtrType::PtrType(Auto<TypeRef> of) : TypeRef(of->pos), of(of) {}

Auto<TypeRef> PtrType::resolve(World &in, const CppName &context) const {
	Auto<PtrType> r = new PtrType(*this);
	r->of = of->resolve(in, context);
	return r;
}

void PtrType::print(wostream &to) const {
	to << of << L"*";
}

RefType::RefType(Auto<TypeRef> of) : TypeRef(of->pos), of(of) {}

Auto<TypeRef> RefType::resolve(World &in, const CppName &context) const {
	Auto<RefType> r = new RefType(*this);
	r->of = of->resolve(in, context);
	return r;
}

void RefType::print(wostream &to) const {
	to << of << L"&";
}

MaybeType::MaybeType(Auto<TypeRef> of) : TypeRef(of->pos) {
	if (this->of = of.as<PtrType>())
		;
	else
		throw Error(L"MAYBE() must contain a pointer.", of->pos);
}

Auto<TypeRef> MaybeType::resolve(World &in, const CppName &context) const {
	Auto<MaybeType> r = new MaybeType(*this);
	r->of = of->resolve(in, context).as<PtrType>();
	if (!r->of)
		throw Error(L"MAYBE content turned into non-pointer.", of->pos);
	return r;
}

void MaybeType::print(wostream &to) const {
	to << L"MAYBE(" << of << L")";
}

NamedType::NamedType(const SrcPos &pos, const CppName &name) : TypeRef(pos), name(name) {}

NamedType::NamedType(const SrcPos &pos, const String &name) : TypeRef(pos), name(name) {}

Size NamedType::size() const {
	throw Error(L"Unknown size of the non-exported type " + name +
				L"\nUse PTR_NOGC or similar if this is the intent.", pos);
}

Auto<TypeRef> NamedType::resolve(World &in, const CppName &context) const {
	map<String, Size>::const_iterator i = in.builtIn.find(name);
	if (i != in.builtIn.end())
		return new BuiltInType(pos, name, i->second);

	Type *t = in.types.findUnsafe(name, context);
	if (t)
		return new ResolvedType(*this, t);
	else
		return new NamedType(*this);
}

void NamedType::print(wostream &to) const {
	to << name;
}

ResolvedType::ResolvedType(const TypeRef &templ, Type *type) : TypeRef(templ), type(type) {}

Size ResolvedType::size() const {
	return type->size();
}

bool ResolvedType::gcType() const {
	return type->heapAlloc();
}

Auto<TypeRef> ResolvedType::resolve(World &in, const CppName &context) const {
	return new ResolvedType(*this, type);
}

void ResolvedType::print(wostream &to) const {
	to << type->name;
}

BuiltInType::BuiltInType(const SrcPos &pos, const String &name, Size size) : TypeRef(pos), name(name), tSize(size) {}

Auto<TypeRef> BuiltInType::resolve(World &in, const CppName &context) const {
	return new BuiltInType(pos, name, tSize);
}

void BuiltInType::print(wostream &to) const {
	to << name;
}

GcArrayType::GcArrayType(const SrcPos &pos, Auto<TypeRef> of) : TypeRef(pos), of(of) {}

Auto<TypeRef> GcArrayType::resolve(World &in, const CppName &context) const {
	Auto<GcArrayType> r = new GcArrayType(*this);
	r->of = of->resolve(in, context);
	return r;
}

Size GcArrayType::size() const {
	throw Error(L"Array<> should only be used as a pointer!", pos);
}

void GcArrayType::print(wostream &to) const {
	to << L"storm::GcArray<" << of << L">";
}

GcDynArrayType::GcDynArrayType(const SrcPos &pos, Auto<TypeRef> of) : TypeRef(pos), of(of) {}

Auto<TypeRef> GcDynArrayType::resolve(World &in, const CppName &context) const {
	Auto<GcDynArrayType> r = new GcDynArrayType(*this);
	r->of = of->resolve(in, context);
	return r;
}

Size GcDynArrayType::size() const {
	throw Error(L"Array<> should only be used as a pointer!", pos);
}

void GcDynArrayType::print(wostream &to) const {
	to << L"storm::GcDynArray<" << of << L">";
}

const UnknownType::ID UnknownType::ids[] = {
	{ L"PTR_NOGC", Size::sPtr, false },
	{ L"PTR_GC", Size::sPtr, true },
	{ L"INT", Size::sInt, false },
};

UnknownType::UnknownType(const String &kind, Auto<TypeRef> of) : TypeRef(of->pos), of(of), id(null) {
	for (nat i = 0; i < ARRAY_COUNT(ids); i++) {
		if (kind == ids[i].name) {
			id = &ids[i];
			break;
		}
	}

	if (!id)
		throw Error(L"Invalid UNKNOWN() declaration. " + kind + L" is not known.", of->pos);
}

Size UnknownType::size() const {
	return id->size;
}

bool UnknownType::gcType() const {
	return id->gc;
}

Auto<TypeRef> UnknownType::resolve(World &in, const CppName &ctx) const {
	return new UnknownType(*this);
}

void UnknownType::print(wostream &to) const {
	to << L"UNKNOWN(" << id->name << L") " << of;
}
