#pragma once
#include "../Storm/Fn.h"
#include "Compiler/Package.h"
#include "Compiler/UrlWithContents.h"
#include "Core/Convert.h"

inline void reloadFile(Package *pkg, const wchar *file, const wchar *contents) {
	Url *url = pkg->url();
	assert(url, L"Attempting to replace virtual package.");
	url = *url / new (pkg) Str(file);
	url = new (pkg) UrlWithContents(url, new (pkg) Str(contents));
	pkg->reload(url);
}

inline void reloadFile(Package *pkg, const wchar *file, const char *contents) {
	Url *url = pkg->url();
	assert(url, L"Attempting to replace virtual package.");
	url = *url / new (pkg) Str(file);
	url = new (pkg) UrlWithContents(url, new (pkg) Str(toWChar(pkg->engine(), contents)));
	pkg->reload(url);
}
