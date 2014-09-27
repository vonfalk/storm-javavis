#include "StdAfx.h"
#include "Debug.h"

#include "LogStream.h"

#include <iomanip>

namespace util {

#ifdef _DEBUG
	static DebugTarget currentDebugTarget = debugToAuto;
#else
	static DebugTarget currentDebugTarget = debugToStdOut;
#endif

	static void createDebugConsole() {
		AllocConsole();
		wchar_t oldTitle[MAX_PATH];
		wchar_t newTitle[MAX_PATH];
		GetConsoleTitle(oldTitle, MAX_PATH);

		wsprintf(newTitle, L"%d/%d", GetTickCount(), GetCurrentProcessId());
		SetConsoleTitle(newTitle);

		Sleep(40);

		HWND wnd = FindWindow(NULL, newTitle);
		SetConsoleTitle(oldTitle);

		SetWindowPos(wnd, null, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
	}

	static void debugConsoleOutput(const wchar_t *data) {
		std::string str = String(data).toChar();
		DWORD result;
		WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), str.c_str(), str.size(), &result, NULL);
	}

	void setDebugTarget(DebugTarget target) {
		if (target == debugToConsole) {
			if (GetStdHandle(STD_OUTPUT_HANDLE) == NULL) {
				// create a console
				createDebugConsole();
			}
		}

		currentDebugTarget = target;
	}
	static void debugOutputFn(const wchar_t *data);

	static void findOutput(const wchar_t *data) {
		HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
		if (h == NULL) {
			// no console, do not create one
			setDebugTarget(debugToVS);
		} else {
			// console found! output to stdout
			setDebugTarget(debugToStdOut);
		}
		debugOutputFn(data);
	}

	static void debugOutputFn(const wchar_t *data) {
		switch (currentDebugTarget) {
			case debugToStdOut:
				std::wcout << data;
				std::wcout.flush();
				break;
			case debugToAuto:
				findOutput(data);
				break;
			case debugToVS:
				OutputDebugString(data);
				break;
			case debugToConsole:
				debugConsoleOutput(data);
				break;
		}
	}

	std::wostream &debugStream() {
		static LogStreamBuffer<void (*)(const wchar_t *), wchar_t> streamBuffer(debugOutputFn);
		static std::wostream stream(&streamBuffer);
		return stream;
	}

	void printLines(const std::string &str) {
		printLines(String(str.c_str()));
	}

	void printLines(const String &str) {
		std::wostream &to = debugStream();
		nat line = 1;
		for (nat i = 0; i < str.size(); i++) {
			to << str[i];
			if (str[i] == '\n') {
				to <<std::setw(3) << line++ << ": ";
			}
		}
	}
}
