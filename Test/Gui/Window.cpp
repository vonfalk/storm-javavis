#include "stdafx.h"
#include "Storm/Fn.h"

BEGIN_TEST(Window, Gui) {

	PLN(L"Showing a simple window. Click the button and it should output things\n"
		L"to the screen. Close it to proceed to more tests.");
	runFn<void>(L"test.ui.simple");

	PLN(L"Trying showing a dialog from a window. This makes sure that 'waitForClose'\n"
		L"works properly when called from the message pump.");
	runFn<void>(L"test.ui.dialog");

	PLN(L"Trying graphics!");
	runFn<void>(L"test.ui.main");

	PLN(L"Trying layers!");
	runFn<void>(L"test.ui.layer");

} END_TEST
