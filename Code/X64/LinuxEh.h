#pragma once

namespace code {
	namespace x64 {
		namespace linux {

			/**
			 * Low-level exception handling interface for Linux (assumes we're using DWARF information
			 * for unwinding).
			 *
			 * The eh_frame section (which is used during stack unwinding) contains a set of
			 * records. Each record is either a CIE record or a FDE record. CIE records describe general
			 * parameters that can be used from multiple FDE records. FDE records describe what happens
			 * in a single function.
			 *
			 * Note that the length is shared between the CIE and FDE records.
			 *
			 * Note: Examine the file unwind-dw2-fde.h/unwind-dw2-fde.c for details on how the
			 * exception handling is implemented on GCC. We will abuse some structures from there.
			 */

			/**
			 * CIE record.
			 */
			struct CIE {
				// Length of this record. If it is 0xFFFFFFFF, the next 8 bytes contain a 64-bit
				// length (not supported in this implementation).
				Nat length;

				// ID of the record. Must be zero.
				Nat id;

				// Version.
				Byte version;

				// String containing information followed by the rest of the data.
				Byte data[1];
			};

			/**
			 * FDE record.
			 */
			struct FDE {
				// Length of this record. If it is 0xFFFFFFFF, the next 8 bytes contain a 64-bit
				// length (not supported in this implementation).
				Nat length;

				// ID of the record. Number of bytes from this field to the start of the CIE record to be used.
				Nat id;
			};
		}
	}
}
