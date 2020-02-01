#pragma once

namespace code {

	/**
	 * Describes a stack frame in a relatively machine-independent manner. Only attempts to describe
	 * enough for stack-unwinding to work.
	 *
	 * Not exposed to Storm as we can not allocate objects on Storm:s heap when exceptions are being
	 * handled.
	 */
	class StackFrame : NoCopy {
	public:
		StackFrame(Nat block);

		// Currently active block.
		Nat block;

		// Compute the offset of the variable, given the offset found in the metadata table.
		virtual void *toPtr(size_t offset) = 0;
	};

}
