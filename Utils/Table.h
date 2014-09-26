#pragma once

#include <vector>

namespace util {

	//Assigns all objects added an unique id (starting from 0), to later store something based on this information in an array.
	//Also ensures that the indices given are constant until the items are removed.
	//
	//NOTE: No data is deleted by this class!

	template <class T>
	class Table {
	public:
		Table();
		virtual ~Table();

        inline T *operator [](nat index) const;

		inline T *at(nat index) const;
		inline void set(nat index, T *v);
		void remove(nat index);
		nat insert(T *i);
		bool find(T *i, nat &found);
		bool contains(T *i);
		inline nat size() const;

		void clear();
	private:
		typedef vector<T *> Contents;
		Contents data;

		void resize();
	};


	template <class T>
	Table<T>::Table() {}

	template <class T>
	Table<T>::~Table() {}

	template <class T>
    T *Table<T>::operator [](nat index) const {
		return data[index];
	}

	template <class T>
	T *Table<T>::at(nat index) const {
		return data[index];
	}

	template <class T>
	void Table<T>::set(nat index, T *v) {
		data[index] = v;
	}

	template <class T>
	void Table<T>::remove(nat index) {
		if (index >= data.size()) return;

		T *d = data[index];
		data[index] = null;

		resize();
	}

	template <class T>
	nat Table<T>::insert(T *ins) {
		for (nat i = 0; i < data.size(); i++) {
			if (data[i] == null) {
				data[i] = ins;
				return i;
			}
		}

		data.push_back(ins);
		return data.size() - 1;
	}

	template <class T>
	bool Table<T>::find(T *item, nat &found) {
		for (nat i = 0; i < data.size(); i++) {
			if (data[i] == item) {
				found = i;
				return true;
			}
		}
		return false;
	}

	template <class T>
	bool Table<T>::contains(T *item) {
		for (nat i = 0; i < data.size(); i++) {
			if (data[i] == item) return true;
		}
		return false;
	}

	template <class T>
	nat Table<T>::size() const {
		return data.size();
	}

	template <class T>
	void Table<T>::resize() {
		while (data.size() > 0 && data.back() == null) data.pop_back();

		// Ensure the vector has shrunk.
		if (data.capacity() - data.size() > 10) {
			data = Contents(data);
		}
	}
}