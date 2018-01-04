#include "stdafx.h"
#include "Storm/Fn.h"

BEGIN_TEST(Window, Gui) {

	PLN(L"Showing a simple window. Click the button and it should output things\n"
		L"to the screen. Close it to proceed to more tests.");
	runFn<void>(S("test.ui.simple"));

	PLN(L"Trying showing a dialog from a window. This makes sure that 'waitForClose'\n"
		L"works properly when called from the message pump.");
	runFn<void>(S("test.ui.dialog"));

	PLN(L"Trying graphics!");
	runFn<void>(S("test.ui.main"));

	PLN(L"Trying layers!");
	runFn<void>(S("test.ui.layer"));

	PLN(L"Trying overlays with other windows inside the frame. Try clicking 'Toggle' a few times.");
	runFn<void>(S("test.ui.overlay"));

	PLN(L"Trying out loading various image formats.");
	runFn<void>(S("test.ui.images"));

} END_TEST
