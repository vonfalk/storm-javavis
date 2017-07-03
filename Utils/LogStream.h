#pragma once

#include <iostream>

namespace util {

	// A std-like class for debug output to visual studio command prompt window (or any output function)

	template <class LogFunction, class Elem = wchar_t>
	class LogStreamBuffer : public std::basic_streambuf<Elem> {
	public:
		LogStreamBuffer(LogFunction fn) : logFn(fn) {
			setBuffer();
		}

	protected:
		typedef typename std::basic_streambuf<Elem>::int_type int_type;
		typedef typename std::basic_streambuf<Elem>::traits_type traits_type;

		Elem *pbase() const;
		Elem *pptr() const;
		Elem *epptr() const;

		virtual int sync() {
			overflow(traits_type::eof());
			return 0;
		}

		virtual int_type overflow(int_type ch) {
			Elem *begin = pbase();
			Elem *end = pptr();

			if (ch != traits_type::eof()) {
				end[0] = Elem(ch);
				end[1] = 0;
			} else {
				end[0] = 0;
			}

			(*logFn)(begin);

			setBuffer();

			return 0;
		}
	private:
		LogFunction logFn;

		enum { bufferSize = 1024 };

		Elem buffer[bufferSize];

		void setBuffer() {
			// room for an extra character as well as a null character
			setp(&buffer[0], &buffer[bufferSize - 3]);
		}

	};

}
