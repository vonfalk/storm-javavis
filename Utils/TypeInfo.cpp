#include "stdafx.h"
#include "TypeInfo.h"

TypeInfo::operator BasicTypeInfo() {
	BasicTypeInfo::Kind k = BasicTypeInfo::ptr;

	if (plain()) {
		switch (kind) {
		case nothing:
			k = BasicTypeInfo::nothing;
			break;
		case signedNr:
			k = BasicTypeInfo::signedNr;
			break;
		case unsignedNr:
			k = BasicTypeInfo::unsignedNr;
			break;
		case floatNr:
			k = BasicTypeInfo::floatNr;
			break;
		case user:
		default:
			k = BasicTypeInfo::user;
			break;
		}
	}

	BasicTypeInfo r = { size, k };
	return r;
}

wostream &operator <<(wostream &to, BasicTypeInfo::Kind k) {
	switch (k) {
	case BasicTypeInfo::signedNr:
		return to << L"signed";
	case BasicTypeInfo::unsignedNr:
		return to << L"unsigned";
	case BasicTypeInfo::floatNr:
		return to << L"float";
	case BasicTypeInfo::user:
		return to << L"user";
	default:
		return to << L"?";
	}
}

wostream &operator <<(wostream &to, const BasicTypeInfo &t) {
	return to << L"{size=" << t.size
			  << L", kind=" << t.kind << L"}";
}

wostream &operator <<(wostream &to, TypeInfo::Kind k) {
	switch (k) {
	case TypeInfo::signedNr:
		return to << L"signed";
	case TypeInfo::unsignedNr:
		return to << L"unsigned";
	case TypeInfo::floatNr:
		return to << L"float";
	case TypeInfo::user:
		return to << L"user";
	default:
		return to << L"?";
	}
}

wostream &operator <<(wostream &to, const TypeInfo &t) {
	return to << L"{size=" << t.size
			  << L", base=" << t.baseSize
			  << L", ref=" << (t.ref ? L"true" : L"false")
			  << L", ptrs=" << t.ptrs
			  << L", kind=" << t.kind << L"}";
}
