#pragma once

#include "TextFile.h"

namespace util {

	class TextWriter {
	public:
		virtual ~TextWriter();

		virtual textfile::Format format() = 0;

		virtual void put(wchar_t ch) = 0;
		virtual void put(const String &str);

		static TextWriter *create(Stream *stream, textfile::Format fmt);
	protected:
		TextWriter(Stream *stream);

		Stream *stream;
	};

	namespace textfile {
		class Utf8Writer : public TextWriter {
		public:
			Utf8Writer(Stream *to);

			virtual textfile::Format format() { return textfile::utf8; };
			virtual void put(wchar_t ch);
		private:
			wchar_t largeCp;

			static inline bool isSurrogate(wchar_t ch) { return (ch & 0xFC00) == 0xD800; };
			static int decodeUtf16(wchar_t high, wchar_t low);
			void encodeUtf8(int utf32);
		};

		class Utf16Writer : public TextWriter {
		public:
			Utf16Writer(Stream *to, bool reverseEndian = false);

			virtual textfile::Format format() { return (reverseEndian ? textfile::utf16rev : textfile::utf16); };

			virtual void put(wchar_t ch);
		private:
			bool reverseEndian;
		};
	}
}