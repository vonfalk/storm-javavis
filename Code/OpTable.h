#pragma once

namespace code {

	// Implements code and macros for convenient creation of tables mapping from
	// an op-code to something else.

	template <class T>
	struct OpEntry {
		OpCode opCode;
		T data;
	};

	template <class T>
	class OpTable {
	public:
		// Create the table.
		OpTable(const OpEntry<T> *entries, nat count) {
			for (nat i = 0; i < count; i++) {
				const OpEntry<T> &e = entries[i];
				data[e.opCode] = e.data;
			}
		}

		// Get the op-code at location 'o'.
		T &operator[](OpCode o) { return data[o]; }

	private:
		// Data.
		T data[op::numOpCodes];
	};


}