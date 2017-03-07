#include "stdafx.h"
#include "Message.h"
#include "MsgName.h"

namespace gui {

	Message::Message(const MSG &src) : msg(src.message), wParam(src.wParam), lParam(src.lParam) {}

	Message::Message(UINT msg, WPARAM wParam, LPARAM lParam) :
		msg(msg), wParam(wParam), lParam(lParam) {}

	void Message::output(wostream &to) const {
		to << msgName(msg) << L" (" << msg << L"), " << wParam << L", " << lParam;
	}

	MsgResult noResult() {
		MsgResult r = { false, 0 };
		return r;
	}

	MsgResult msgResult(LRESULT code) {
		MsgResult r = { true, code };
		return r;
	}

}
