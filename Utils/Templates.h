#pragma once

namespace templates {

	template <class T>
	class Identity {
	public:
		inline T &operator()(T &a) { return a; }
	};

}