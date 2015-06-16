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
	class ValueData : public Printable {
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

	protected:
		virtual void output(wostream &to) const;
	};


	/**
	 * Template magic for finding the Value of a C++ type.
	 */

	template <class T>
	struct LookupValue {
		static Type *type(Engine &e) {
			return T::stormType(e);
		}
	};

	template <class T>
	struct LookupValue<T *> {
		static Type *type(Engine &e) {
			return T::stormType(e);
		}
	};

	template <class T>
	struct LookupValue<T &> {
		static Type *type(Engine &e) {
			return T::stormType(e);
		}
	};

	template <>
	struct LookupValue<Int> {
		static Type *type(Engine &e) {
			return intType(e);
		}
	};

	template <>
	struct LookupValue<Nat> {
		static Type *type(Engine &e) {
			return natType(e);
		}
	};

	template <>
	struct LookupValue<Byte> {
		static Type *type(Engine &e) {
			return byteType(e);
		}
	};

	template <>
	struct LookupValue<Bool> {
		static Type *type(Engine &e) {
			return boolType(e);
		}
	};

	// Helper...
	bool isClass(Type *t);

	template <class T>
	ValueData value(typename EnableIf<!IsVoid<T>::value, Engine>::t &e) {
		bool isRef = !typeInfo<T>().plain() || IsAuto<T>::v;
		Type *t = LookupValue<T>::type(e);
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
