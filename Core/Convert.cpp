#include "stdafx.h"
#include "Convert.h"
#include "Utf.h"

namespace storm {

#define WRITE(x)								\
	do {										\
		if (out < maxCount)						\
			to[out] = (x);						\
		out++;									\
	} while (false)

#define TERMINATE()								\
	WRITE(0);									\
	if (maxCount > 0)							\
		to[maxCount - 1] = 0

#define WRAP_CONVERT(NAME, FROM, TO, TO_GC)								\
	GcArray<TO> *NAME(Engine &e, const FROM *from) {					\
		size_t space = convert(from, (TO *)null, 0);					\
		GcArray<TO> *r = runtime::allocArray<TO>(e, &TO_GC, space);		\
		convert(from, r->v, space);										\
		return r;														\
	}

	size_t convert(const char *from, wchar *to, size_t maxCount) {
		size_t out = 0;

		const char *at = from;
		while (*at) {
			nat left;
			nat cp = utf8::firstData(*at, left);
			at++;

			for (nat i = 0; i < left; i++) {
				if (utf8::isCont(*at)) {
					cp = utf8::addCont(cp, *at);
					at++;
				} else {
					cp = replacementChar;
					break;
				}
			}

			// Encode...
			if (utf16::split(cp)) {
				WRITE(utf16::splitLeading(cp));
				WRITE(utf16::splitTrailing(cp));
			} else {
				WRITE(wchar(cp));
			}
		}

		TERMINATE();
		return out;
	}

	size_t convert(const char *from, size_t fromCount, wchar *to, size_t maxCount) {
		size_t out = 0;

		size_t at = 0;
		while (at < fromCount) {
			nat left;
			nat cp = utf8::firstData(from[at++], left);

			for (nat i = 0; i < left; i++) {
				if (at < fromCount && utf8::isCont(from[at])) {
					cp = utf8::addCont(cp, from[at++]);
				} else {
					cp = replacementChar;
					break;
				}
			}

			// Encode...
			if (utf16::split(cp)) {
				WRITE(utf16::splitLeading(cp));
				WRITE(utf16::splitTrailing(cp));
			} else {
				WRITE(wchar(cp));
			}
		}

		TERMINATE();
		return out;
	}

	size_t convert(const wchar *from, char *to, size_t maxCount) {
		size_t out = 0;

		const wchar *at = from;
		byte buffer[utf8::maxBytes];
		while (*at) {
			wchar now = *at++;
			nat cp = 0;
			if (utf16::leading(now)) {
				if (utf16::trailing(*at))
					cp = utf16::assemble(now, *at++);
				else
					cp = replacementChar;
			} else {
				cp = now;
			}

			// Encode...
			nat count = 0;
			byte *buf = utf8::encode(cp, buffer, &count);
			for (nat i = 0; i < count; i++)
				WRITE(buf[i]);
		}

		TERMINATE();
		return out;
	}

	WRAP_CONVERT(toWChar, char, wchar, wcharArrayType);
	WRAP_CONVERT(toChar, wchar, char, byteArrayType);

#ifdef POSIX

	size_t convert(const wchar_t *from, wchar *to, size_t maxCount) {
		size_t out = 0;

		for (const wchar_t *at = from; *at; at++) {
			nat cp = *at;
			if (utf16::split(cp)) {
				WRITE(utf16::splitLeading(cp));
				WRITE(utf16::splitTrailing(cp));
			} else {
				WRITE(wchar(cp));
			}
		}

		TERMINATE();
		return out;
	}

	WRAP_CONVERT(toWChar, wchar_t, wchar, wcharArrayType);

#endif

}
