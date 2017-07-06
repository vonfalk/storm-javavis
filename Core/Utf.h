#pragma once

namespace storm {

	// Codepoint for the 'replacement character', which is used whenever encoding or decoding of text fails.
	static const nat16 replacementChar = 0xFFFD;

	/**
	 * Utf16 helpers.
	 */
	namespace utf16 {

		// Is this a leading character?
		static inline bool leading(nat16 ch) {
			return (ch & 0xFC00) == 0xD800;
		}

		// Is this a trailing character?
		static inline bool trailing(nat16 ch) {
			return (ch & 0xFC00) == 0xDC00;
		}

		// Assemble a leading and a trailing char into one codepoint.
		static inline nat assemble(nat16 lead, nat16 trail) {
			nat r = nat(lead & 0x3FF) << nat(10);
			r |= nat(trail & 0x3FF);
			r += 0x10000;
			return r;
		}

		// Valid codepoint?
		static inline bool valid(nat cp) {
			if (cp >= 0x110000)
				return false;
			if (cp >= 0xD800 && cp < 0xE000)
				return false;
			return true;
		}

		// Should this codepoint be split?
		static inline bool split(nat cp) {
			return cp >= 0x10000;
		}

		// Get the leading codepoint. Return 0 if should not be split.
		static inline nat16 splitLeading(nat cp) {
			if (!split(cp) || !valid(cp))
				return 0;

			cp -= 0x10000;
			cp >>= 10;
			return nat16(0xD800 + (cp & 0x3FF));
		}

		// Get the trailing codepoint.
		static inline nat16 splitTrailing(nat cp) {
			if (!valid(cp)) {
				return replacementChar;
			} else if (split(cp)) {
				cp -= 0x10000;
				return nat16(0xDC00 + (cp & 0x3FF));
			} else {
				return cp;
			}
		}

	}

	/**
	 * Utf8 helpers.
	 */
	namespace utf8 {

		// Is this a continuation byte?
		static inline bool isCont(byte c) {
			return (c & 0xC0) == 0x80;
		}

		// Get the data from a continuation byte.
		static inline byte contData(byte c) {
			return c & 0x3F;
		}

		// Add another continuation byte to the character.
		static inline nat addCont(nat prev, byte cont) {
			return (prev << 6) | contData(cont);
		}

		// Get information about the first byte of a codepoint. Returns the initial
		// codepoint-data. 'remaining' indicates the number of remaining continuation bytes that
		// shall be appended using 'addCont'.
		static inline nat firstData(byte c, nat &remaining) {
			if ((c & 0x80) == 0) {
				remaining = 0;
				return nat(c);
			} else if ((c & 0xC0) == 0x80) {
				// Continuation from before: error!
				remaining = 0;
				return replacementChar;
			} else if ((c & 0xE0) == 0xC0) {
				remaining = 1;
				return nat(c & 0x1F);
			} else if ((c & 0xF0) == 0xE0) {
				remaining = 2;
				return nat(c & 0x0F);
			} else if ((c & 0xF8) == 0xF0) {
				remaining = 3;
				return nat(c & 0x07);
			} else if ((c & 0xFC) == 0xF8) {
				remaining = 4;
				return nat(c & 0x03);
			} else if ((c & 0xFE) == 0xFC) {
				remaining = 5;
			    return nat(c & 0x01);
			} else {
				remaining = 0;
				return replacementChar;
			}
		}

		// Largest number of bytes required for a single codepoint in UTF-8. Including null terminator.
		static const nat maxBytes = 9;

		// Encode a codepoint in UTF-8. 'out' is assumed to be at least 'maxBytes' entries
		// large. 'count' optionally returns the number of bytes used inside the buffer. A pointer
		// to the first written byte is returned from the function.
		static inline byte *encode(nat cp, byte *buffer, nat *count) {
			if (cp < 0x80) {
				// Fast path: 1 byte codepoints.
				buffer[0] = byte(cp);
				buffer[1] = 0;
				if (count)
					*count = 1;
				return buffer;
			}

			// Output multiple bytes...
			byte *at = buffer + maxBytes - 1;
			*at = 0;
			nat leadingBits = 6;
			nat bytes = 0;
			do {
				// Output the least significant 6 bits.
				*--at = byte(0x80 | (cp & 0x3F));
				cp = cp >> 6;
				bytes++;
				leadingBits--;
			} while (cp >= (Nat(1) << leadingBits));

			// Output the first byte indicating the length of the codepoint.
			*--at = byte((0xFF << (leadingBits + 1)) | cp);
			bytes++;

			if (count)
				*count = bytes;
			return at;
		}

	}

}
