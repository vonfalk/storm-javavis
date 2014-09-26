// stdafx.cpp : source file that includes just the standard includes
// Spel.pch will be the pre-compiled header
// stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"

#ifndef NO_MFC
#include <Shlwapi.h>

bool validateFilename(const CString &name) {
	return (name.FindOneOf(L"\\/:*?<>|") < 0);
}


double strToDouble(const CString &str) {
	wchar_t *end;
	return wcstod(str, &end);
}

int strToInt(const CString &str) {
	wchar_t *end;
	return wcstol(str, &end, 10);
}

int hexStrToInt(const CString &str) {
	wchar_t *end;
	if (str.Left(2) == L"0x") {
		return wcstol(str.Mid(2), &end, 16);
	} else {
		return wcstol(str, &end, 16);
	}
}

CString toString(int i) {
	CString t;
	t.Format(L"%d", i);
	return t;
}

CString toString(nat i) {
	CString t;
	t.Format(L"%u", i);
	return t;
}

CString toString(int64 i) {
	CString t;
	t.Format(L"%lld", i);
	return t;
}

CString toString(nat64 i) {
	CString t;
	t.Format(L"%llu", i);
	return t;
}

CString toString(double i) {
	CString t;
	t.Format(L"%f", i);
	return t;
}

CString toString(const char *cstr) {
	size_t sz;
	mbstowcs_s(&sz, null, 0, cstr, strlen(cstr));
	if (sz <= 0) return L"";

	wchar_t *wchs = new wchar_t[sz + 1];
	wchs[sz] = 0;
	mbstowcs_s(&sz, wchs, sz + 1, cstr, strlen(cstr));

	CString ret = wchs;
	delete []wchs;
	return ret;
}

CString toHexString(nat i, bool prefix) {
	CString t;
	if (prefix) {
		t.Format(L"0x%08X", i);
	} else {
		t.Format(L"%08X", i);
	}
	return t;
}

CString toHexString(const void *i, bool prefix) {
	return toHexString(nat(i), prefix);
}
#endif

bool getAsyncKeyState(nat key) { //Hur är det just nu?
	return (GetAsyncKeyState(key) & 0x8000) != 0;
}

bool getKeyState(nat key) { //Hur var det när nuv. meddelande skickades?
	return (GetKeyState(key) & 0x8000) != 0;
}


bool positive(float f) {
	return f >= 0.0f;
}


#ifdef WIN32

void atomicIncrement(volatile nat &v) {
	InterlockedIncrement((volatile LONG *)&v);
}

void atomicDecrement(volatile nat &v) {
	InterlockedDecrement((volatile LONG *)&v);
}

#else
#error "atomicIncrement and atomicDecrement are only supported on Windows"
#endif