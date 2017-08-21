#include "stdafx.h"
#include "TypeInfo.h"

wostream &operator <<(wostream &to, TypeKind::T k) {
	switch (k) {
	case TypeKind::signedNr:
		return to << L"signed";
	case TypeKind::unsignedNr:
		return to << L"unsigned";
	case TypeKind::floatNr:
		return to << L"float";
	case TypeKind::boolVal:
		return to << L"bool";
	case TypeKind::userTrivial:
		return to << L"user(trivial)";
	case TypeKind::userComplex:
		return to << L"user(complex)";
	default:
		return to << L"?";
	}
}

TypeInfo::operator BasicTypeInfo() {
	BasicTypeInfo::Kind k = TypeKind::ptr;

	if (plain()) {
		k = kind;
	}

	BasicTypeInfo r = { nat(size), k };
	return r;
}

wostream &operator <<(wostream &to, const BasicTypeInfo &t) {
	return to << L"{size=" << t.size
			  << L", kind=" << t.kind << L"}";
}

wostream &operator <<(wostream &to, const TypeInfo &t) {
	return to << L"{size=" << t.size
			  << L", base=" << t.baseSize
			  << L", ref=" << (t.ref ? L"true" : L"false")
			  << L", ptrs=" << t.ptrs
			  << L", kind=" << t.kind << L"}";
}
