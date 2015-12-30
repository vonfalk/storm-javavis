#pragma once

namespace storm {

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

		// Should this codepoint be split?
		static inline bool split(nat cp) {
			return cp >= 0x10000;
		}

		// Get the leading codepoint.
		static inline nat16 splitLeading(nat cp) {
			cp -= 0x10000;
			cp >>= 10;
			return nat16(0xD800 + (cp & 0x3FF));
		}

		// Get the trailing codepoint.
		static inline nat16 splitTrailing(nat cp) {
			cp -= 0x10000;
			return nat16(0xDC00 + (cp & 0x3FF));
		}

		// Valid codepoint?
		static inline bool valid(nat cp) {
			if (cp >= 0x110000)
				return false;
			if (cp >= 0xD800 && cp < 0xE000)
				return false;
			return true;
		}

	}
}
