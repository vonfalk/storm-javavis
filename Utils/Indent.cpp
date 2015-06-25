#include "stdafx.h"
#include "Indent.h"

static const wchar indentStr[] = L"  ";

Indent::Indent(std::wostream &stream) :
	owner(stream),
	to(stream.rdbuf()),
	startOfLine(true) {
	owner.rdbuf(this);
	setp(buffer, buffer + bufferSize - 1);
}


Indent::~Indent() {
	sync();
	owner.rdbuf(to);
}

int Indent::sync() {
	overflow(traits_type::eof());
	return 0;
}

Indent::int_type Indent::overflow(int_type ch) {
	wchar *begin = pbase();
	wchar *end = pptr();

	if (ch != traits_type::eof())
		*end++ = wchar(ch);

	for (wchar *at = begin; at != end; at++) {
		if (*at == '\n') {
			startOfLine = true;
		} else if (startOfLine) {
			startOfLine = false;
			if (at != begin)
				to->sputn(begin, at - begin);
			to->sputn(indentStr, ARRAY_COUNT(indentStr) - 1);
			begin = at;
		}
	}

	if (begin != end)
		to->sputn(begin, end - begin);

	setp(buffer, buffer + bufferSize - 1);
	return 0;
}
