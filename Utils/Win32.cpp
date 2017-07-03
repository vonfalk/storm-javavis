#include "stdafx.h"
#include "Win32.h"

#ifdef POSIX

int _tmain(int argc, const wchar_t *argv[]);

int main(int argc, const char *argv[]) {
	vector<String> args(argv, argv+argc);
	vector<const wchar_t *> c_args(argc);
	for (int i = 0; i < argc; i++)
		c_args[i] = args[i].c_str();

	return _tmain(argc, &c_args[0]);
}

#endif
