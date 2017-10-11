#pragma once

#ifdef POSIX
namespace code {
	namespace x64 {

		/**
		 * Low-level exception handling interface for Linux. Assumes we're using DWARF
		 * information for unwinding on GCC. CLang might be compatible enough so that this code
		 * works unmodified, but most likely the most GCC specific hooks will need to be rewritten.
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

			// Set the 'id' field properly.
			void setCie(CIE *to);
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


#ifdef GCC

		/**
		 * Struct holding sorted records.
		 */
		struct SortedRecords {
			Record *original; // We know this is the case for our usage.
			size_t count;
			Record **sorted;
		};

		/**
		 * Struct compatible with 'object' from 'libgcc'. We can ignore much of the struct, but
		 * we need to be able to read and write the unions 'u' and 's'.
		 */
		struct GCCObject {
			void *pc_begin;
			void *tbase;
			void *dbase;

			union {
				Record *single;
				Record **array;
				SortedRecords *sorted;
			} u;

			union {
				struct {
					unsigned long sorted : 1;
					unsigned long from_array : 1;
					unsigned long mixed_encoding : 1;
					unsigned long encoding : 8;
					unsigned long count : 21;
				} b;
				size_t all;
			} s;

			// There are a maximum of two more pointers here, but depending on compilation flags
			// for 'libgcc' their layout is not known here. Therefore, we can not use them (and
			// we do not need to either).
			void *fde_end;
			void *next;
		};

#endif

	}
}
#endif
