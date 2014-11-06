#pragma once

// A stack-allocated byte array which can be casted to other types
// easily.
// NOTE: Do not use with types containing ctors and/or dtors unless you're
// really knowing what you're doing!
template <int MaxSize>
class Blob {
public:
	byte data[MaxSize];
	nat currSize;

	Blob::Blob() {
		zeroMem(data);
		currSize = 0;
	}

	bool operator ==(const Blob &other) const {
		if (currSize != other.currSize) return false;
		for (nat i = 0; i < currSize; i++) {
			if (data[i] != other.data[i]) return false;
		}
		return true;
	}

	inline bool operator !=(const Blob &other) const { return !(*this == other); }

	template <class T>
	T get() const {
		assert(sizeof(T) <= MaxSize);
		assert(currSize == sizeof(T));
		T tmp;
		memcpy(&tmp, data, sizeof(T));
		return tmp;
	}

	template <class T>
	void set(const T &p) {
		PLN_IF("INVALID SIZE: " << sizeof(T) << " vs " << MaxSize, sizeof(T) > MaxSize);
		assert(sizeof(T) <= MaxSize);
		currSize = sizeof(T);
		memcpy(data, &p, sizeof(T));
	}
};


