#include "stdafx.h"
#include "Access.h"

wostream &operator <<(wostream &to, Access a) {
	switch (a) {
	case aPublic:
		return to << L"public";
	case aProtected:
		return to << L"protected";
	case aPrivate:
		return to << L"private";
	default:
		return to << L"?";
	}
}
