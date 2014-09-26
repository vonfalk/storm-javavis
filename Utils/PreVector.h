#pragma once

namespace util {

	//An implementation of a preallocated vector, to save malloc:s.
	//This implementation pre-allocates N elements in the vector and
	//grows afterwards if neccessary. This allows the first N elements
	//to be automatic storage, thereby eliminating the need for separate
	//malloc:s. Usage example can be seen in the events in GSL.
	//
	//T needs to have an empty constructor, a copy ctor as well as an assignment operator.
	template <class T, int N = 10>
	class PreVector {
	public:
		PreVector();
		template <int Z>
		PreVector(const PreVector<T, Z> &other);
		~PreVector();

		template <int Z>
		PreVector &operator =(const PreVector<T, Z> &other);

		inline const T &operator[](nat i) const {
			ASSERT(i < mySize);
			return i < N ? fixedData[i] : dynamicData[i - N];
		}

		inline T &operator[](nat i) {
			ASSERT(i < mySize);
			return i < N ? fixedData[i] : dynamicData[i - N];
		}

		inline nat size() const { return mySize; }

		void push_back(const T &a);

		void clear();
	private:
		nat mySize, allocSize;
		T fixedData[N];
		T *dynamicData;

		void ensureSize(nat s);
		void growTo(nat s);

		static const nat GROW_STEP = 10;
	};

	//Implementation...
	template <class T, int N>
	PreVector<T, N>::PreVector() : mySize(0), allocSize(N), dynamicData(0) {}

	template <class T, int N> template <int Z>
	PreVector<T, N>::PreVector(const PreVector<T, Z> &other) : size(0), dynamicData(0) {
		*this = other;
	}

	template <class T, int N>
	PreVector<T, N>::~PreVector() {
		clear();
	}

	template <class T, int N>
	void PreVector<T, N>::clear() {
		delete []dynamicData;
		dynamicData = null;
		mySize = 0;
	}

	template <class T, int N> template <int Z>
	PreVector<T, N> &PreVector<T, N>::operator =(const PreVector<T, Z> &other) {
		clear();
		ensureSize(other.size());

		for (nat i = 0; i < other.size(); i++) {
			(*this)[i] = other[i];
		}

		return *this;
	}

	template <class T, int N>
	void PreVector<T, N>::ensureSize(nat s) {
		if (allocSize >= s) {
			mySize = s;
			return;
		}

		nat newSize = s;
		if (newSize - allocSize < GROW_STEP) newSize = allocSize + GROW_STEP;

		if (newSize > N) {
			if (dynamicData) {
				T *newData = new T[newSize - N];
				for (nat i = 0; i < min(newSize, mySize) - N; i++) {
					newData[i] = dynamicData[i];
				}
				delete []dynamicData;
				dynamicData = newData;
			} else {
				dynamicData = new T[newSize - N];
			}
		}
		mySize = s;
		allocSize = newSize;
	}

	template <class T, int N>
	void PreVector<T, N>::push_back(const T &a) {
		ensureSize(mySize + 1);
		(*this)[mySize - 1] = a;
	}
}