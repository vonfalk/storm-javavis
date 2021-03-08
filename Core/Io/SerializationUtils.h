#pragma once
#include "Serialization.h"
#include "Core/Array.h"

namespace storm {

	/**
	 * This file contains some utilities for using the serialization interface in C++.
	 *
	 * In particular, this file contains:
	 * - Serialization/deserialization of primitive types.
	 * - Serialization/deserialization of arrays/maps/etc. (currently only when they contain objects).
	 */
	template <class T>
	struct Serialize {};

	template <class T>
	struct Serialize<T *> {
		static T *read(ObjIStream *from) {
			return (T *)from->readClass(StormInfo<T *>::type(from->engine()));
		}
		static void write(T *value, ObjOStream *to) {
			value->write(to);
		}
	};

	// Helper struct for the primitives.
	template <class T, StoredId id> // , void (OStream::*write)(T)>
	struct PrimitiveSerialize {
		static T read(ObjIStream *from) {
			T result;
			from->readPrimitiveValue(id, &result);
			return result;
		}

		static void write(T value, ObjOStream *to) {
			to->startPrimitive(id);
			to->to->writeT(value);
			to->end();
		}
	};

	template <>
	struct Serialize<Bool> : PrimitiveSerialize<Bool, boolId> {};
	template <>
	struct Serialize<Byte> : PrimitiveSerialize<Byte, byteId> {};
	template <>
	struct Serialize<Int> : PrimitiveSerialize<Int, intId> {};
	template <>
	struct Serialize<Nat> : PrimitiveSerialize<Nat, natId> {};
	template <>
	struct Serialize<Long> : PrimitiveSerialize<Long, longId> {};
	template <>
	struct Serialize<Word> : PrimitiveSerialize<Word, wordId> {};
	template <>
	struct Serialize<Float> : PrimitiveSerialize<Float, floatId> {};
	template <>
	struct Serialize<Double> : PrimitiveSerialize<Double, doubleId> {};

	// Array of something (currently only objects).
	template <class T>
	struct Serialize<Array<T> *> {
		static Array<T> *read(ObjIStream *from) {
			return (Array<T> *)from->readClass(StormInfo<Array<T> *>::type(from->engine()));
		}

		static void write(Array<T> *value, ObjOStream *to) {
			if (!to->startClass(StormInfo<Array<T> *>::type(to->engine()), value))
				return;

			to->to->writeNat(value->count());
			for (Nat i = 0; i < value->count(); i++) {
				Serialize<T>::write(value->at(i), to);
			}

			to->end();
		}
	};

}
