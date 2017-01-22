#pragma once

namespace stormgui {

	template <class T>
	class ComPtr {
	public:
		ComPtr() : v(null) {}

		ComPtr(T *o) : v(o) {}

		ComPtr(const ComPtr<T> &o) : v(o.v) {
			if (v)
				v->AddRef();
		}

		~ComPtr() {
			::release(v);
		}

		ComPtr<T> &operator =(const ComPtr<T> &o) {
			::release(v);
			v = o.v;
			if (v)
				v->AddRef();
			return *this;
		}

		T *v;
	};

}
