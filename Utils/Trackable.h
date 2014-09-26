#pragma once

namespace util {

	// This is a class intended for tracking changes to the contained
	// value. T is supposed to be some kind of immutable type.
	template <class T>
	class Trackable {
	public:
		inline Trackable() : changed(false) {}
		inline Trackable(const T &initTo) : changed(false), value(initTo) {}

		inline operator const T() const { return value; }
		inline operator T() { return value; }

		inline Trackable<T> &operator =(const T &v) {
			value = v;
			changed = true;
			return *this;
		}
		inline Trackable<T> &operator =(const Trackable<T> &o) {
			value = o.value;
			changed = true;
			return *this;
		}

		inline bool isChanged() {
			bool t = changed;
			changed = false;
			return t;
		}
	private:
		T value;
		bool changed;
	};

}