#include "stdafx.h"
#include "License.h"

License::License(const String &id, const String &pkg, const String &cond, const String &title, const String &author, const String &body) :
	id(id), pkg(pkg), condition(cond), title(title), author(author), body(body) {}
