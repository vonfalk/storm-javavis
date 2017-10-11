#include "stdafx.h"
#include "LinuxEh.h"

#ifdef POSIX

namespace code {
	namespace x64 {

		// Personality function for Storm (the signature is not neccessarily correct).
		void stormPersonality(int version, int actions, size_t exception_class, void *info, void *state) {
			// TODO!
		}

		/**
		 * Definitions of some useful DWARF constants.
		 */
#define DW_CFA_advance_loc        0x40 // + delta
#define DW_CFA_offset             0x80 // + register
#define DW_CFA_restore            0xC0 // + register
#define DW_CFA_nop                0x00
#define DW_CFA_set_loc            0x01
#define DW_CFA_advance_loc1       0x02
#define DW_CFA_advance_loc2       0x03
#define DW_CFA_advance_loc4       0x04
#define DW_CFA_offset_extended    0x05
#define DW_CFA_restore_extended   0x06
#define DW_CFA_undefined          0x07
#define DW_CFA_same_value         0x08
#define DW_CFA_register           0x09
#define DW_CFA_remember_state     0x0a
#define DW_CFA_restore_state      0x0b
#define DW_CFA_def_cfa            0x0c
#define DW_CFA_def_cfa_register   0x0d
#define DW_CFA_def_cfa_offset     0x0e
#define DW_CFA_def_cfa_expression 0x0f
#define DW_CFA_expression         0x10
#define DW_CFA_offset_extended_sf 0x11
#define DW_CFA_def_cfa_sf         0x12
#define DW_CFA_def_cfa_offset_sf  0x13
#define DW_CFA_val_offset         0x14
#define DW_CFA_val_offset_sf      0x15
#define DW_CFA_val_expression     0x16

#define DW_EH_PE_absptr 0x00

		/**
		 * DWARF helpers.
		 */

		// Write a LEB128 value (unsigned).
		void putULeb128(byte *to, nat &pos, nat value) {
			while (value >= 0x80) {
				to[pos++] = (value & 0x7F) | 0x80;
				value >>= 7;
			}
			to[pos++] = byte(value);
		}

		// Write a LEB128 value (signed).
		void putSLeb128(byte *to, nat &pos, int value) {
			nat src = value;
			nat bits = value;
			if (value < 0) {
				bits = ~bits; // make 'positive'
				bits <<= 1; // make sure to get at least one sign bit in the output.
			}
			while (bits >= 0x80) {
				to[pos++] = (src & 0x7F) | 0x80;
				src >>= 7;
				bits >>= 7;
			}
			to[pos++] = src & 0x7F;
		}

		// Write a plain pointer.
		void putPtr(byte *to, nat &pos, const void *value) {
			const void **t = (const void **)(to + pos);
			*t = value;
			pos += sizeof(const void **);
		}

		/**
		 * Records
		 */

		void Record::setLen(void *end) {
			char *from = (char *)(void *)this;
			char *to = (char *)end;
			length = to - (from + 4);
		}

		void CIE::init() {
			id = 0;
			version = 1;
			nat pos = 0;
			data[pos++] = 'z'; // There is a size of all augmentation data.
			data[pos++] = 'R'; // FDE encoding of addresses.
			data[pos++] = 'P'; // Personality function.
			data[pos++] = 0;

			putULeb128(data, pos, 1); // code alignment
			putSLeb128(data, pos, -8); // data alignment factor
			putULeb128(data, pos, 16); // where is the return address stored (here, it is a virtual register)?

			putULeb128(data, pos, 2 + sizeof(void *)); // size of augmentation data.
			data[pos++] = DW_EH_PE_absptr; // absolute addresses.
			data[pos++] = DW_EH_PE_absptr; // encoding of the personality function
			putPtr(data, pos, address(&stormPersonality));

			data[pos++] = DW_CFA_def_cfa;
			putULeb128(data, pos, 7); // rsp
			putULeb128(data, pos, 8); // offset 8
			data[pos++] = DW_CFA_offset + 16;
			putSLeb128(data, pos, 1); // rip saved at cfa-8
		}

		void FDE::setCie(CIE *to) {
			char *me = (char *)&cieOffset;
			cieOffset = me - (char *)to;
		}

		/**
		 * Chunk.
		 */

		void Chunk::init() {
			// Clear everything.
			memset(this, DW_CFA_nop, sizeof(Chunk));

			// Fill in the 'length' fields of all members.
			header.setLen(&functions[0]);
			for (Nat i = 0; i < CHUNK_COUNT - 1; i++)
				functions[i].setLen(&functions[i+1]);
			functions[CHUNK_COUNT-1].setLen(&end);
			end.length = 0;

			// Set the CIE of all functions.
			for (Nat i = 0; i < CHUNK_COUNT; i++)
				functions[i].setCie(&header);

			// Initialize the CIE.
			header.init();
		}

	}
}

#endif
