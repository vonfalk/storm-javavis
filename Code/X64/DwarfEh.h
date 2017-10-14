#pragma once
#include "../Reg.h"

namespace code {
	namespace x64 {
		STORM_PKG(core.asm.x64);

		/**
		 * Generation of DWARF unwinding information to properly support exceptions in the generated
		 * code. Assumes that DWARF(2) unwinding data is used in the compiler. The eh_frame section
		 * (which is used during stack unwinding) contains a set of records. Each record is either a
		 * CIE record or a FDE record. CIE records describe general parameters that can be used from
		 * multiple FDE records. FDE records describe what happens in a single function.
		 *
		 * Note that the length is shared between the CIE and FDE records.
		 *
		 * Note: Examine the file unwind-dw2-fde.h/unwind-dw2-fde.c for details on how the
		 * exception handling is implemented on GCC. We will abuse some structures from there.
		 */

		/**
		 * Configuration:
		 * CHUNK_COUNT - Maximum number of functions inside of each chunk.
		 * CIE_DATA - Maximum number of bytes in the 'data' section of a CIE record.
		 * FDE_DATA - Maximum number of bytes in the 'data' section of a FDE record.
		 */
		static const nat CHUNK_COUNT = 10; // 100;
		static const nat CIE_DATA = 24 - 1;
		static const nat FDE_DATA = 24;

		/**
		 * A generic DWARF record. The fields in here line up so that they match 'CIE' and 'FDE'.
		 */
		struct Record {
			// Length of this record. If it is 0xFFFFFFFF, the next 8 bytes contain a 64-bit
			// length (not supported in this implementation).
			Nat length;

			// Set the length so that this object ends at 'end'.
			void setLen(void *end);
		};

		/**
		 * CIE record.
		 *
		 * A record that contains things that are the same for most functions. Therefore, it is
		 * only stored once per table.
		 */
		struct CIE : Record {
			// Id (zero).
			Nat id;

			// Version.
			Byte version;

			// String containing information followed by the rest of the data.
			Byte data[CIE_DATA];

			// Initialize.
			void init();
		};

		/**
		 * FDE record.
		 *
		 * Stores data specific to a function.
		 */
		struct FDE : Record {
			// Offset of the CIE relative to here.
			Nat cieOffset;

			// Data section of the FDE.
			Byte data[FDE_DATA];

			// Access the first few members in the FDE. We always use absolute pointers, so we know their layout!

			// Start address of the code.
			const void *& codeStart() {
				return *(const void **)&data[0];
			}
			// Total number of bytes of code.
			size_t &codeSize() {
				return *(size_t *)&data[8];
			}
			// Size of augmenting data.
			Byte &augSize() {
				return data[16];
			}

			// First free offset in 'data'.
			Nat firstFree() const {
				return 17;
			}

			// Set the 'id' field properly.
			void setCie(CIE *to);
		};

		class ArrayStream;

		/**
		 * Generate information about functions, later used by the exception system on some
		 * platforms.
		 */
		class FnInfo {
			STORM_VALUE;
		public:
			FnInfo(Engine &e);

			// Note that the prolog has been executed. The prolog is expected to use ptrFrame as usual.
			void prolog(Nat pos);

			// Note that the epilog has been executed.
			void epilog(Nat pos);

			// Note that a register has been stored to the stack (for preservation).
			void preserve(Nat pos, Reg reg, Offset offset);

		private:
			// The data emitted.
			GcArray<Byte> *data;

			// Last position which we encoded something at.
			Nat lastPos;

			// Go to 'pos'.
			void advance(ArrayStream &to, Nat pos);
		};

		/**
		 * Store exception data for a set number of functions.
		 */
		struct Chunk {
			// CIE forming the 'header' of this chunk.
			CIE header;

			// The payload of this chunk.
			FDE functions[CHUNK_COUNT];

			// An empty Record to indicate the end of the FDEs.
			Record end;

			// Initialize a chunk.
			void init();
		};


	}
}
