#pragma once

#include <iostream>

void waitForReturn();
#ifndef NO_MFC
std::wostream &operator <<(std::wostream &to, const CString &from);
std::wostream &operator <<(std::wostream &to, const CStringA &from);
std::ostream &operator <<(std::ostream &to, const CString &from);
std::ostream &operator <<(std::ostream &to, const CStringA &from);
#endif
