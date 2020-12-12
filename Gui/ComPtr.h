#pragma once

namespace gui {

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

		T *operator ->() const {
			return v;
		}

		operator bool() const {
			return v != null;
		}

		ComPtr<T> &operator =(const ComPtr<T> &o) {
			::release(v);
			v = o.v;
			if (v)
				v->AddRef();
			return *this;
		}

		// Clear (set to null)
		void clear() {
			::release(v);
			v = null;
		}

		T *v;
	};

}
