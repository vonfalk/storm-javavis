#include "stdafx.h"
#include "CppType.h"
#include "Exception.h"
#include "World.h"

CppType::CppType(const SrcPos &pos) : pos(pos), constType(false) {}

ArrayType::ArrayType(Auto<CppType> of) : CppType(of->pos), of(of) {}

Size ArrayType::size() const {
	throw Error(L"Array<> should only be used as a pointer!", pos);
}

Auto<CppType> ArrayType::resolve(World &in) const {
	Auto<ArrayType> r = new ArrayType(*this);
	r->of = of->resolve(in);
	return r;
}

void ArrayType::print(wostream &to) const {
	to << L"storm::Array<" << of << L">";
}

MapType::MapType(Auto<CppType> k, Auto<CppType> v) : CppType(k->pos), k(k), v(v) {}

Size MapType::size() const {
	throw Error(L"Map<> should only be used as a pointer!", pos);
}

Auto<CppType> MapType::resolve(World &in) const {
	Auto<MapType> r = new MapType(*this);
	r->k = k->resolve(in);
	r->v = k->resolve(in);
	return r;
}

void MapType::print(wostream &to) const {
	to << L"storm::Map<" << k << L", " << v << L">";
}

TemplateType::TemplateType(const SrcPos &pos, const CppName &name) : CppType(pos), name(name) {}

Size TemplateType::size() const {
	throw Error(L"Templates are unsupported in general!", pos);
}

Auto<CppType> TemplateType::resolve(World &in) const {
	if (name == L"Array" && params.size() == 1) {
		return new ArrayType(params[0]->resolve(in));
	} else if (name == L"Map" && params.size() == 2) {
		return new MapType(params[0]->resolve(in), params[1]->resolve(in));
	} else {
		throw Error(L"Unknown template type: " + toS(this), pos);
	}
}

void TemplateType::print(wostream &to) const {
	to << name << L"<";
	join(to, params, L", ");
	to << L">";
}

PtrType::PtrType(Auto<CppType> of) : CppType(of->pos), of(of) {}

Auto<CppType> PtrType::resolve(World &in) const {
	Auto<PtrType> r = new PtrType(*this);
	r->of = of->resolve(in);
	return r;
}

void PtrType::print(wostream &to) const {
	to << of << L"*";
}

RefType::RefType(Auto<CppType> of) : CppType(of->pos), of(of) {}

Auto<CppType> RefType::resolve(World &in) const {
	Auto<RefType> r = new RefType(*this);
	r->of = of->resolve(in);
	return r;
}

void RefType::print(wostream &to) const {
	to << of << L"&";
}

MaybeType::MaybeType(Auto<CppType> of) : CppType(of->pos) {
	if (this->of = of.as<PtrType>())
		;
	else
		throw Error(L"MAYBE() must contain a pointer.", of->pos);
}

Auto<CppType> MaybeType::resolve(World &in) const {
	Auto<MaybeType> r = new MaybeType(*this);
	r->of = of->resolve(in).as<PtrType>();
	if (!r->of)
		throw Error(L"MAYBE content turned into non-pointer.", of->pos);
	return r;
}

void MaybeType::print(wostream &to) const {
	to << L"MAYBE(" << of << L")";
}

NamedType::NamedType(const SrcPos &pos, const CppName &name) : CppType(pos), name(name) {}

NamedType::NamedType(const SrcPos &pos, const String &name) : CppType(pos), name(name) {}

Size NamedType::size() const {
	throw Error(L"Unknown size of the non-exported type " + name, pos);
}

Auto<CppType> NamedType::resolve(World &in) const {
	Type *t = in.findTypeUnsafe(name);
	if (t)
		return new ResolvedType(*this, t);
	else
		return new NamedType(*this);
}

void NamedType::print(wostream &to) const {
	to << name;
}

ResolvedType::ResolvedType(const CppType &templ, Type *type) : CppType(templ), type(type) {}

Size ResolvedType::size() const {
	return type->size();
}

bool ResolvedType::gcType() const {
	return !type->valueType;
}

Auto<CppType> ResolvedType::resolve(World &in) const {
	return new ResolvedType(*this, type);
}

void ResolvedType::print(wostream &to) const {
	to << type->name;
}
