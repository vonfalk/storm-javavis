#pragma once
#include "OpCode.h"

namespace code {

	// Implements code and macros for convenient creation of tables mapping from
	// an op-code to something else.

	template <class T>
	struct OpEntry {
		op::Code opCode;
		T data;
	};

	template <class T>
	class OpTable {
	public:
		// Create the table.
		OpTable(const OpEntry<T> *entries, nat count) {
			for (nat i = 0; i < ARRAY_COUNT(data); i++)
				data[i] = T();

			for (nat i = 0; i < count; i++) {
				const OpEntry<T> &e = entries[i];
				// assert(data[e.opCode] == null);
				data[e.opCode] = e.data;
			}
		}

		// Get the op-code at location 'o'.
		T &operator[](op::Code o) { return data[o]; }

	private:
		// Data.
		T data[op::numOpCodes];
	};


	/**
	 * Contains a subset of op-codes.
	 */
	class OpSet {
	public:
		// Create.
		inline OpSet(const op::Code *opCodes, nat count) {
			for (nat i = 0; i < ARRAY_COUNT(data); i++)
				data[i] = false;

			for (nat i = 0; i < count; i++) {
				op::Code c = opCodes[i];
				data[c] = true;
			}
		}

		// Get the op-code at 'o'.
		inline bool operator[](op::Code o) { return data[o]; }

	private:
		// Data.
		bool data[op::numOpCodes];
	};

}
