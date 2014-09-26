#pragma once

#include "Function.h"

namespace util {
	template <class T>
	class Value;

	// Base class for all values to enable convenient destruction.
	class ValueBase : NoCopy {
	public:
		virtual ~ValueBase();

		virtual ValueBase *clone() = 0;

		template <class T>
		bool isType() const {
			return dynamic_cast<const Value<T> *>(this) != null;
		}

		template <class T>
		Value<T> *getType() {
			ASSERT(isType<T>());
			return (Value<T> *)this;
		}

		template <class T>
		const Value<T> *getType() const {
			ASSERT(isType<T>());
			return (const Value<T> *)this;
		}
	};

	// A class that encapsulates a value of some kind. Either it is a direct pointer
	// to the value itself, or it is a setter and a getter for the value.
	template <class T>
	class Value : public ValueBase {
	public:
		virtual ~Value();

		// Create a value stored inside this value.
		Value(const T &data);

		// Create a value with a data pointer.
		Value(T *data);

		// Create a value with a getter and a setter.
		Value(const Function<T> &getter, const Function<void, T> &setter);
		Value(const Function<const T &> &getter, const Function<void, const T &> &setter);

		// Get the value.
		operator T() const;

		// Set the value.
		Value<T> &operator =(const T &);

		// Clone
		virtual Value<T> *clone();
	private:
		bool owningData;
		T *data;
		Function<T> *plainGetter;
		Function<void, T> *plainSetter;
		Function<const T &> *refGetter;
		Function<void, const T &> *refSetter;

		void zeroMembers();
	};

	// Convenient creation.
	template <class T>
	Value<T> *value(const T &data) {
		return new Value<T>(data);
	}

	template <class T>
	Value<T> *value(T *data) {
		return new Value<T>(data);
	}

	template <class T>
	Value<T> *value(const Function<T> &getter, const Function<void, T> &setter) {
		return new Value<T>(getter, setter);
	}

	template <class T>
	Value<T> *value(const Function<const T &> &getter, const Function<void, const T &> &setter) {
		return new Value<T>(getter, setter);
	}

	// Create from this ptr and a setter and a getter.

	template <class T, class C>
	Value<T> *value(C *me, T (C:: *getter)(), void (C:: *setter)(T)) {
		return new Value<T>(memberVoidFn(me, getter), memberFn(me, setter));
	}

	template <class T, class C>
	Value<T> *value(C *me, T (C:: *getter)() const, void (C:: *setter)(T)) {
		return new Value<T>(memberVoidFn(me, getter), memberFn(me, setter));
	}

	template <class T, class C>
	Value<T> *valueRef(C *me, const T &(C:: *getter)() const, void (C:: *setter)(const T &)) {
		return new Value<T>(memberVoidFn(me, getter), memberFn(me, setter));
	}

	template <class T, class C>
	Value<T> *valueRef(C *me, const T &(C:: *getter)(), void (C:: *setter)(const T &)) {
		return new Value<T>(memberVoidFn(me, getter), memberFn(me, setter));
	}

	template <class T>
	Value<T>::~Value() {
		if (owningData) del(data);
		delete plainGetter;
		delete plainSetter;
		delete refGetter;
		delete refSetter;
	}

	template <class T>
	void Value<T>::zeroMembers() {
		owningData = false;
		data = null;
		plainGetter = null;
		plainSetter = null;
		refGetter = null;
		refSetter = null;
	}

	template <class T>
	Value<T>::Value(const T &data) {
		zeroMembers();
		owningData = true;
		this->data = new T(data);
	}

	template <class T>
	Value<T>::Value(T *data) {
		zeroMembers();
		this->data = data;
	}

	template <class T>
	Value<T>::Value(const Function<T> &getter, const Function<void, T> &setter) {
		zeroMembers();
		this->plainGetter = new Function<T>(getter);
		this->plainSetter = new Function<void, T>(setter);
	}

	template <class T>
	Value<T>::Value(const Function<const T &> &getter, const Function<void, const T &> &setter) {
		zeroMembers();
		this->refGetter = new Function<const T&>(getter);
		this->refSetter = new Function<void, const T&>(setter);
	}

	template <class T>
	Value<T>::operator T() const {
		if (data) {
			return *data;
		} else if (plainGetter) {
			return (*plainGetter)();
		} else {
			return (*refGetter)();
		}
	}

	template <class T>
	Value<T> &Value<T>::operator =(const T &v) {
		if (data) {
			*data = v;
		} else if (plainSetter) {
			(*plainSetter)(v);
		} else {
			(*refSetter)(v);
		}
		return *this;
	}

	template <class T>
	Value<T> *Value<T>::clone() {
		if (data) {
			if (owningData) {
				return new Value<T>(*data);
			} else {
				return new Value<T>(data);
			}
		} else if (plainSetter && plainGetter) {
			return new Value<T>(*plainGetter, *plainSetter);
		} else {
			return new Value<T>(*refGetter, *refSetter);
		}
	}
}