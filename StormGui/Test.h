#pragma once

namespace stormgui {

	class Test : public Object {
		STORM_CLASS;
	public:
		STORM_CTOR Test();

		Str *STORM_FN test();

		ArrayP<Str> *STORM_FN testArray();

		FnPtr<Str *, Int> *STORM_FN testPtr();

	private:
		Str *CODECALL testPtr(Int v);
	};

}
