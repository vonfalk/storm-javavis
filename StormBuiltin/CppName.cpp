#include "stdafx.h"
#include "CppName.h"
#include "Parse.h"

CppName CppName::read(Tokenizer &tok) {
	vector<String> parts;
	parts.push_back(tok.next());
	while (true) {
		String t = tok.peek();
		if (t == L"<") {
			skipTemplate(tok);
		} else if (t == L"::") {
			tok.next();
			parts.push_back(tok.next());
		} else {
			break;
		}
	}
	return CppName(parts);
}

void CppName::output(wostream &to) const {
	join(to, parts, L"::");
}


void CppScope::push(bool type, const String &name, const CppName &super) {
	Part p = { type, 0, name, super };
	parts.push_back(p);
}

void CppScope::push() {
	if (!parts.empty())
		parts.back().depth++;
}

void CppScope::pop() {
	if (parts.empty())
		std::wcout << L"Warning, unbalanced braces." << endl;
	if (!parts.empty() && parts.back().depth-- == 0)
		parts.pop_back();
}

CppName CppScope::cppName() const {
	vector<String> r(parts.size());
	for (nat i = 0; i < parts.size(); i++)
		r[i] = parts[i].name;
	return CppName(r);
}

bool CppScope::isType() const {
	return !parts.empty() && parts.back().type;
}

CppName CppScope::super() const {
	if (!parts.empty())
		return parts.back().super;
	return CppName();
}

String CppScope::name() const {
	if (!parts.empty())
		return parts.back().name;
	return L"";
}

void CppScope::output(wostream &to) const {
	to << cppName();
	if (isType())
		to << L"(super: " << super() << L")";
}
