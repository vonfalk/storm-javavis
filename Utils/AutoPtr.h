#pragma once

namespace util {

	template <class T>
	class AutoPtr {
	public:
		AutoPtr();
		AutoPtr(T *o);
		AutoPtr(const AutoPtr<T> &other);
		~AutoPtr();

		AutoPtr<T> &operator =(const AutoPtr<T> &o);

		inline operator T *() const;
		inline T &operator *() const;
		inline T *operator ->() const;

		inline T *v();
	private:
		T *ptr;
	};

	class RefCount {
	public:
		RefCount() : refs(0) {};
		virtual ~RefCount() {};

		inline void addRef() const {
			InterlockedIncrement(&refs);
		};
		inline void release() const {
			long r = InterlockedDecrement(&refs);
			if (r <= 0) delete this;
		};
	private:
		mutable long refs; //Allow using const objects as well.
	};

	template <class T>
	AutoPtr<T>::AutoPtr() : ptr(null) {}

	template <class T>
	AutoPtr<T>::AutoPtr(T *o) : ptr(o) {
		if (ptr) ptr->addRef();
	}

	template <class T>
	AutoPtr<T>::AutoPtr(const AutoPtr<T> &other) : ptr(other.ptr) {
		if (ptr) ptr->addRef();
	}

	template <class T>
	AutoPtr<T>::~AutoPtr() {
		if (ptr) ptr->release();
	}

	template <class T>
	AutoPtr<T> &AutoPtr<T>::operator =(const AutoPtr<T> &o) {
		if (ptr) ptr->release();
		ptr = o.ptr;
		if (ptr) ptr->addRef();
		return *this;
	}

	template <class T>
	AutoPtr<T>::operator T *() const {
		return ptr;
	}

	template <class T>
	T &AutoPtr<T>::operator *() const {
		return *ptr;
	}

	template <class T>
	T *AutoPtr<T>::operator ->() const {
		return ptr;
	}
}
