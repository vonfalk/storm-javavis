#include "stdafx.h"
#include "Parse.h"
#include "CppName.h"

void skipTemplate(Tokenizer &tok) {
	nat c = 1;
	tok.expect(L"<");
	while (c > 0) {
		switch (tok.next()[0]) {
		case '>':
			c--;
			break;
		case '<':
			c++;
			break;
		}
	}
}


String parsePkg(Tokenizer &tok) {
	tok.expect(L"(");

	std::wostringstream r;
	bool first = true;
	while (tok.peek() != L")") {
		if (!first) {
			tok.expect(L".");
			r << L".";
		}

		r << tok.next();
		first = false;
	}

	tok.expect(L")");
	return r.str();
}

CppName findSuper(Tokenizer &tok) {
	CppName r;

	if (tok.peek() != L":")
		return r;
	tok.next();

	while (true) {
		String mode = tok.peek();
		if (mode == L"public") {
			// Want it!
			tok.next();
			r = CppName::read(tok);
		} else if (mode == L"private" || mode == L"protected") {
			// consume and ignore
			tok.next();
			CppName::read(tok);
		} else {
			// Consume
			CppName::read(tok);
		}

		if (tok.peek() == L"{")
			break;
		tok.expect(L",");
	}

	return r;
}

