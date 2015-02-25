#include "stdafx.h"
#include "TypeInfo.h"

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
