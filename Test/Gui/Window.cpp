#include "stdafx.h"
#include "Storm/Fn.h"

BEGIN_TEST(Window, Gui) {

	PLN(L"Showing a simple window with graphics. Close it to proceed.");
	runFn<void>(S("tests.ui.simple"));

	PLN(L"Trying showing a dialog from a window. This makes sure that 'waitForClose'\n"
		L"works properly when called from the message pump.");
	runFn<void>(S("tests.ui.dialog"));

	PLN(L"Trying graphics!");
	runFn<void>(S("tests.ui.main"));

	PLN(L"Trying layers!");
	runFn<void>(S("tests.ui.layer"));

	PLN(L"Trying overlays with other windows inside the frame. Try clicking 'Toggle' a few times.");
	runFn<void>(S("tests.ui.overlay"));

	PLN(L"Trying out loading various image formats.");
	runFn<void>(S("tests.ui.images"));

} END_TEST
