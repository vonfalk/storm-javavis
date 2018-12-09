#pragma once

namespace storm {

	/**
	 * Convert numbers to/from strings using regular C++ IO-streams.
	 *
	 * Used inside the StrBuf and Str classes.
	 *
	 * Note: These assume we're dealing with ASCII, and therefore we don't have to do anything fancy
	 * when converting to/from UTF-16.
	 */
	template <Nat size>
	class StdIBuf : public std::basic_streambuf<wchar_t> {
	public:
		StdIBuf(const wchar *buffer, Nat count) {
			count = ::min(count, size);
			for (Nat i = 0; i < count; i++)
				data[i] = buffer[i];
			setg(data, data, data + count);
		}

	protected:
		virtual int_type underflow() {
			return std::char_traits<wchar_t>::eof();
		}

	private:
		wchar_t data[size];
	};

	template <Nat size>
	class StdOBuf : public std::basic_streambuf<wchar_t> {
	public:
		StdOBuf() {
			memset(data, 0, sizeof(data));
			setp(data, data + size - 1);
		}

		void copy(wchar *to, Nat count) {
			count = ::min(count, size) - 1;
			for (Nat i = 0; i < count; i++)
				to[i] = data[i];
			to[count] = 0;
		}

	protected:
		virtual int_type overflow(int_type) {
			return std::char_traits<wchar>::eof();
		}

	private:
		wchar_t data[size];
	};

}
