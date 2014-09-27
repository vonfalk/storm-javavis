#pragma once

#include <iostream>

class Printable {
public:
	virtual ~Printable();

	String toS() const;
protected:
	virtual void output(std::wostream &to) const = 0;

	friend std::wostream &operator <<(std::wostream &to, const Printable &output);
};

inline String toS(const Printable &o) { return o.toS(); }
