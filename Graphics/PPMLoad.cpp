#include "stdafx.h"
#include "PPMLoad.h"
#include "Image.h"

namespace graphics {

	bool isPPM(IStream *file) {
		const char *spec[] = {
			"P",
			"123456",
			" \n\r\t",
		};

		const Nat len = ARRAY_COUNT(spec);
		Buffer buffer = file->peek(storm::buffer(file->engine(), len));
		if (!buffer.full())
			return false;

		for (Nat i = 0; i < len; i++) {
			bool ok = false;
			for (const char *at = spec[i]; *at; at++) {
				ok |= byte(*at) == buffer[i];
			}

			if (!ok)
				return false;
		}

		return true;
	}

	// Is this a whitespace character?
	static bool whitespace(Byte ch) {
		switch (ch) {
		case ' ':
		case '\n':
		case '\r':
		case '\t':
			return true;
		default:
			return false;
		}
	}

	// Is this a numeric character?
	static bool numeric(Byte ch) {
		return ch >= '0' && ch <= '9';
	}

	struct State {
		IStream *src;
		Buffer buffer;
		Nat pos;

		State(IStream *src) :
			src(src),
			buffer(src->read(1024)),
			pos(0) {}

		// Get the next byte in the file.
		Byte next() {
			if (pos >= buffer.filled()) {
				pos = 0;
				buffer.filled(0);
				buffer = src->read(buffer);

				if (buffer.empty())
					throw new (src) ImageLoadError(S("Unexpected end of stream!"));
			}

			return buffer[pos++];
		}

		// Get the next character, ignoring comments.
		Byte nextText() {
			Byte r;
			for (r = next(); r == '#'; r = next()) {
				// In a comment. Skip until newline.
				while (next() != '\n')
					;
			}
			return r;
		}

		// Get the next number (in ASCII) from the file.
		Nat nextNum() {
			// Skip whitespace.
			Byte ch;
			while (whitespace(ch = nextText()))
				;

			// Read the number.
			if (!numeric(ch))
				throw new (src) ImageLoadError(S("Not a number!"));

			Nat result = 0;
			while (numeric(ch)) {
				result *= 10;
				result += ch - '0';
				ch = nextText();
			}

			return result;
		}
	};

	struct Header {
		// Mode: We use 1, 2 and 3 regardless if binary or ascii.
		Byte mode;

		// Size.
		Nat width, height;
	};

	template <class T>
	Image *loadMonoPPM(State &src, const Header &header, T read) {
		Image *out = new (src.src) Image(header.width, header.height);

		for (Nat y = 0; y < header.height; y++) {
			for (Nat x = 0; x < header.width; x++) {
				Float c = read.next(src);
				out->set(x, y, Color(c, c, c));
			}
			read.flush();
		}

		return out;
	}

	template <class T>
	Image *loadColorPPM(State &src, const Header &header, T read) {
		Image *out = new (src.src) Image(header.width, header.height);

		for (Nat y = 0; y < header.height; y++) {
			for (Nat x = 0; x < header.width; x++) {
				Float r = read.next(src);
				Float g = read.next(src);
				Float b = read.next(src);
				out->set(x, y, Color(r, g, b));
			}
			read.flush();
		}

		return out;
	}

	template <class T>
	Image *loadPPM(State &src, const Header &header) {
		Nat maxval = 0;

		switch (header.mode) {
		case 1:
			return loadMonoPPM(src, header, typename T::Mono());
		case 2:
			maxval = src.nextNum();
			return loadMonoPPM(src, header, typename T::Multi(maxval));
		case 3:
			maxval = src.nextNum();
			return loadColorPPM(src, header, typename T::Multi(maxval));
		default:
			return null;
		}
	}

	struct Raw {
		struct Multi {
			Float maxval;
			Bool multibyte;

			Multi(Nat maxval) : maxval(Float(maxval)), multibyte(maxval > 255) {}

			Float next(State &src) const {
				Nat result = src.next();
				if (multibyte)
					result = (result << 8) | src.next();
				return Float(result) / maxval;
			}

			void flush() const {}
		};

		struct Mono {
			Byte data;
			Byte fill;

			Mono() : data(0), fill(0) {}

			Float next(State &src) {
				if (fill == 0) {
					data = src.next();
					fill = 8;
				}

				fill--;
				return Float(~(data >> fill) & 0x1);
			}

			void flush() {
				fill = 0;
			}
		};
	};

	struct Ascii {
		struct Multi {
			Float maxval;

			Multi(Nat maxval) : maxval(Float(maxval)) {}

			Float next(State &src) const {
				return Float(src.nextNum()) / maxval;
			}

			void flush() const {}
		};

		struct Mono {
			Float next(State &src) const {
				return Float(src.nextNum());
			}

			void flush() const {}
		};
	};

	Image *loadPPM(IStream *src, const wchar *&error) {
		State s(src);

		error = S("Not a supported PPM file.");
		if (s.next() != 'P')
			return null;

		Header header;

		header.mode = s.next();
		if (header.mode >= '1' && header.mode <= '6')
			header.mode -= '0';
		else
			return null;

		if (!whitespace(s.next()))
			return null;

		header.width = s.nextNum();
		header.height = s.nextNum();

		if (header.mode > 3) {
			header.mode -= 3;
			return loadPPM<Raw>(s, header);
		} else {
			return loadPPM<Ascii>(s, header);
		}
	}

}
