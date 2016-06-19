#include "stdafx.h"
#include "CppType.h"
#include "Exception.h"

CppType::CppType(const SrcPos &pos) : pos(pos), constType(false) {}

ArrayType::ArrayType(Auto<CppType> of) : CppType(of->pos), of(of) {}

void ArrayType::print(wostream &to) const {
	to << L"storm::Array<" << of << L">";
}

MapType::MapType(Auto<CppType> k, Auto<CppType> v) : CppType(k->pos), k(k), v(v) {}

void MapType::print(wostream &to) const {
	to << L"storm::Map<" << k << L", " << v << L">";
}

TemplateType::TemplateType(const SrcPos &pos, const CppName &name) : CppType(pos), name(name) {}

void TemplateType::print(wostream &to) const {
	to << name << L"<";
	join(to, params, L", ");
	to << L">";
}

PtrType::PtrType(Auto<CppType> of) : CppType(of->pos), of(of) {}

void PtrType::print(wostream &to) const {
	to << of << L"*";
}

RefType::RefType(Auto<CppType> of) : CppType(of->pos), of(of) {}

void RefType::print(wostream &to) const {
	to << of << L"&";
}

MaybeType::MaybeType(Auto<CppType> of) : CppType(of->pos) {
	if (this->of = of.as<PtrType>())
		;
	else
		throw Error(L"MAYBE() must contain a pointer.", of->pos);
}

void MaybeType::print(wostream &to) const {
	to << L"MAYBE(" << of << L")";
}

NamedType::NamedType(const SrcPos &pos, const CppName &name) : CppType(pos), name(name) {}

NamedType::NamedType(const SrcPos &pos, const String &name) : CppType(pos), name(name) {}

void NamedType::print(wostream &to) const {
	to << name;
}

