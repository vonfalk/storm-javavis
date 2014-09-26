#pragma once

namespace util {

	template <class T>
	class Pointer {
	public:
		Pointer();
		Pointer(T* p, bool owningPtr);
		~Pointer();

		Pointer<T> &operator =(Pointer<T> &other); //Transfers ownership

		operator T*() { return p; };
		operator const T*() const { return p; };
		T* operator ->() { return p; };
		const T* operator ->() const { return p; };

		T* get() { return p; }
		const T* get() const { return p; };

		void set(T* p, bool owningPtr);

		inline T &operator *();
		inline T& operator *() const;
	private:
		T* p;
		bool owned;
		Pointer(Pointer &other); //Does not seem to be called properly for some reason.
	};

	template <class T>
	Pointer<T>::Pointer() : p(null), owned(false) {}

	template <class T>
	Pointer<T>::Pointer(T* p, bool owningPtr) : p(p), owned(owningPtr) {}

 	template <class T>
 	Pointer<T>::Pointer(Pointer<T> &other) : p(other.p), owned(other.owned) {
 		other.owned = false;
 	}

	template <class T>
	Pointer<T> &Pointer<T>::operator =(Pointer<T> &other) {
		if (p) if (owned) delete p;
		p = other.p;
		owned = other.owned;
		other.owned = false;

		return *this;
	}

	template <class T>
	Pointer<T>::~Pointer() {
		if (owned) if (p) delete p;
	}

	template <class T>
	T &Pointer<T>::operator *() {
		return *p;
	}

	template <class T>
	T&Pointer<T>::operator *() const {
		return *p;
	}

	template <class T>
	void Pointer<T>::set(T* p, bool owningPtr) {
		if (owned) if (this->p) delete this->p;

		this->p = p;
		owned = owningPtr;
	}

}