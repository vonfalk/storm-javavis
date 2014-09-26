#include "stdafx.h"
#include "ConUtils.h"

#include <conio.h>

void waitForReturn() {
	//while (_getch() != 13) {}
}

#ifndef NO_MFC
std::wostream &operator <<(std::wostream &to, const CString &from) {
	return to << from.GetString();
}

std::wostream &operator <<(std::wostream &to, const CStringA &from) {
	return to << CString(from);
}

std::ostream &operator <<(std::ostream &to, const CString &from) {
	return to << CStringA(from);
}

std::ostream &operator <<(std::ostream &to, const CStringA &from) {
	return to << from.GetString();
}
#endif
