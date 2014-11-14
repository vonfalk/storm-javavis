#pragma once

#include <iostream>

class Printable {
public:
	virtual ~Printable();

protected:
	virtual void output(std::wostream &to) const = 0;

	friend std::wostream &operator <<(std::wostream &to, const Printable &output);
};

String toS(const Printable &from);
