#pragma once

#include "Register.h"
#include "Listing.h"

namespace code {

	// Computes and tracks the register usage for instructions in a listing.
	// Each entry tracks the registers that needs to be preserved before each instruction.
	// Ignores ptrStack and ptrFrame.
	class UsedRegisters {
	public:
		// Create from the contents in 'src'.
		UsedRegisters(const Listing &src);

		// Get register usage for instruction 'i'.
		const Registers &operator[](nat i) const { return used[i]; }

		// Get all used registers.
		const Registers &allUsed() const { return all; }
	private:
		// All used registers.
		Registers all;

		// Used registers on each line from 'src'.
		vector<Registers> used;

		// Update the data in 'all' and 'used'.
		void findRegisters(const Listing &src);

		// Update 'used' so that it contains all registers read by this instruction and
		// all following instructions under the assumption that 'used' contains the registers
		// required from after 'instr'. Returns all registers written to.
		Registers processInstr(const Instruction &instr, Registers &used);
	};

}

