#pragma once

#include <iostream>

namespace util {

	class Printable {
	public:
		virtual ~Printable();

		String toString() const;
	protected:
		virtual void output(std::wostream &to) const = 0;

		friend std::wostream &operator <<(std::wostream &to, const Printable &output);
	};

}

inline String toS(const util::Printable &o) { return o.toString(); }