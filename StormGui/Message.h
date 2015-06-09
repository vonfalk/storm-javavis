#pragma once

namespace stormgui {

	/**
	 * Wrapper around the windows MSG struct.
	 */
	class Message {
	public:
		Message(const MSG &msg);
		Message(UINT msg, WPARAM wParam, LPARAM lParam);

		UINT msg;
		WPARAM wParam;
		LPARAM lParam;

		// May be extended in the future.
	};

	/**
	 * Result from a message handling function. Returns if there is a response availiable or not as
	 * well as the actual response.
	 */
	struct MsgResult {
		bool any;
		LRESULT result;
	};

	MsgResult noResult();
	MsgResult msgResult(LRESULT result);

}
