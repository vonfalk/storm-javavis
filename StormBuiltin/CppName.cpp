#include "stdafx.h"
#include "CppName.h"
#include "Parse.h"
#include "Type.h"

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

CppName CppName::parent() const {
	if (parts.size() >= 1)
		return vector<String>(parts.begin(), parts.end() - 1);
	else
		return vector<String>();
}

CppName CppName::operator +(const CppName &o) const {
	vector<String> r = parts;
	r.insert(r.end(), o.parts.begin(), o.parts.end());
	return r;
}

CppName CppName::operator +(const String &o) const {
	vector<String> r = parts;
	r.push_back(o);
	return r;
}

bool CppName::operator <(const CppName &o) const {
	return this->parts < o.parts;
}

void CppName::output(wostream &to) const {
	join(to, parts, L"::");
}

bool CppName::isObject() const {
	if (parts.size() != 2)
		return false;
	return parts[0] == L"storm" && parts[1] == L"Object";
}

CppType CppType::read(Tokenizer &tok) {
	CppType t;

	if (tok.peek() == L"Par") {
		tok.next();
		tok.expect(L"<");

		t = CppType::read(tok);
		t.isPar = true;

		tok.expect(L">");

		return t;
	}

	bool isArray = false, isArrayP = false;
	if (tok.peek() == L"Array")
		isArray = true;
	if (tok.peek() == L"ArrayP")
		isArrayP = true;

	if (isArray || isArrayP) {
		tok.next();
		tok.expect(L"<");

		t = CppType::read(tok);
		t.isArray = isArray;
		t.isArrayP = isArrayP;

		tok.expect(L">");
		return t;
	}

	if (tok.peek() == L"const") {
		t.isConst = true;
		tok.next();
	}

	t.type = CppName::read(tok);

	if (tok.peek() == L"*") {
		t.isPtr = true;
		tok.next();
	} else if (tok.peek() == L"&") {
		t.isRef = true;
		tok.next();
	}

	return t;
}

void CppType::output(wostream &to) const {
	if (isPar)
		to << L"storm::Par<";
	if (isArray)
		to << L"storm::Array<";
	if (isArrayP)
		to << L"storm::ArrayP<";
	if (isConst)
		to << L"const ";
	to << type;
	if (isArray)
		to << L">";
	if (isArrayP)
		to << L">";
	if (isPtr)
		to << L" *";
	else if (isRef)
		to << L" &";
	if (isPar)
		to << L">";
}

bool CppType::isVoid() const {
	return type.parts.size() == 1 && type.parts[0] == L"void" && !isPtr;
}

CppType CppType::tVoid() {
	CppType t;
	t.type.parts.push_back(L"void");
	return t;
}

bool CppType::isTypePtr() const {
	return type.parts.size() == 1 && type.parts[0] == L"Type" && isPtr;
}

CppType CppType::typePtr() {
	CppType t;
	t.type = CppName(vector<String>(1, L"Type"));
	t.isPtr = true;
	assert(t.isTypePtr());
	return t;
}

CppType CppType::fullName(const Types &t, const CppName &scope) const {
	CppType c = *this;
	c.type = t.find(type, scope).cppName;
	return c;
}


void CppScope::push(bool type, const String &name, const CppName &super) {
	Part p = { type, 0, name, super };
	parts.push_back(p);
}

void CppScope::push() {
	if (parts.empty())
		rootDepth++;
	else
		parts.back().depth++;
}

void CppScope::pop() {
	if (parts.empty()) {
		if (rootDepth == 0)
			std::wcout << L"Warning, unbalanced braces." << endl;
		else
			rootDepth--;
	} else if (parts.back().depth-- == 0) {
		parts.pop_back();
	}
}

CppName CppScope::cppName() const {
	vector<String> r(parts.size());
	for (nat i = 0; i < parts.size(); i++)
		r[i] = parts[i].name;
	return CppName(r);
}

CppName CppScope::scopeName() const {
	if (isType())
		return cppName().parent();
	else
		return cppName();
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
