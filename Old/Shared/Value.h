#pragma once
#include "Utils/Printable.h"
#include "Utils/TypeInfo.h"
#include "Utils/Templates.h"

namespace storm {

	class Type;

	/**
	 * Shared version of the Value class. This is currently not exported to Storm, it is only
	 * provided to allow templated classes to work.
	 */
	class ValueData {
	public:
		// Create the 'null' value.
		ValueData();

		// Create a value based on a type.
		ValueData(Type *type, bool ref = false);

		// The type referred.
		Type *type;

		// Reference? Not a good idea for return values.
		bool ref;

		// Type equality.
		bool operator ==(const ValueData &o) const;
		inline bool operator !=(const ValueData &o) const { return !(*this == o); }
	};

	// Output.
	wostream &operator <<(wostream &to, const ValueData &data);


	/**
	 * Template magic for finding the Value of a C++ type.
	 */

	template <class T>
	struct StormType {
		static Type *type(Engine &e) {
			return T::stormType(e);
		}
	};

	template <class T>
	struct StormType<T *> {
		static Type *type(Engine &e) {
			return T::stormType(e);
		}
	};

	template <class T>
	struct StormType<T &> {
		static Type *type(Engine &e) {
			return T::stormType(e);
		}
	};

	template <>
	struct StormType<Int> {
		static Type *type(Engine &e) {
			return intType(e);
		}
	};

	template <>
	struct StormType<Nat> {
		static Type *type(Engine &e) {
			return natType(e);
		}
	};

	template <>
	struct StormType<Long> {
		static Type *type(Engine &e) {
			return longType(e);
		}
	};

	template <>
	struct StormType<Word> {
		static Type *type(Engine &e) {
			return wordType(e);
		}
	};

	template <>
	struct StormType<Byte> {
		static Type *type(Engine &e) {
			return byteType(e);
		}
	};

	template <>
	struct StormType<Float> {
		static Type *type(Engine &e) {
			return floatType(e);
		}
	};

	template <>
	struct StormType<Bool> {
		static Type *type(Engine &e) {
			return boolType(e);
		}
	};

	template <>
	struct StormType<void> {
		static Type *type(Engine &e) {
			return null;
		}
	};

	// Helper...
	bool isClass(Type *t);

	// Get the storm type from a type in C++.
	template <class T>
	Type *stormType(Engine &e) {
		return StormType<T>::type(e);
	}

	template <class T>
	ValueData value(typename EnableIf<!IsVoid<T>::value, Engine>::t &e) {
		bool isRef = !typeInfo<T>().plain() || IsAuto<T>::v;
		Type *t = StormType<T>::type(e);
		if (isClass(t)) {
			assert(isRef, "Class type tried to be used by value!");
			isRef = false;
		}
		return ValueData(t, isRef);
	}

	// Special case for void.
	template <class T>
	ValueData value(typename EnableIf<IsVoid<T>::value, Engine>::t &e) {
		return ValueData();
	}

	// Get parameters for the type T.
	vector<ValueData> typeParams(const Type *t);

#ifdef VISUAL_STUDIO
	// This function uses a VS specific extension for variable arguments.
	vector<ValueData> valDataList(nat count, ...);
#else
#error "Define valDataList for C++11 here"
#endif

}
