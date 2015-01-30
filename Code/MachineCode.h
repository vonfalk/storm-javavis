#pragma once

#include "Value.h"
#include "Output.h"
#include "Arena.h"
#include "OpCode.h"

// All currently known specializations of the backend are included here.
#include "X86/MachineCodeX86.h"

namespace code {

	class Instruction;
	class Binary;

	namespace machine {

		// Get the name of a backend-specified register (r >= 0x100)
		const wchar_t *name(Register r);

		// Struct describing the state required for code generation. The
		// only requirements for the state is that it has an empty constructor. This
		// is also the only assumption other code may make about the state type.

		// typedef X State

		//////////////////////////////////////////////////////////////////////////
		// Code transform
		//////////////////////////////////////////////////////////////////////////

		// Transform an entire Listing. "owner" is the object to be called to handle exceptions.
		Listing transform(const Listing &from, const Binary *owner);

		//////////////////////////////////////////////////////////////////////////
		// Code generation
		//////////////////////////////////////////////////////////////////////////

		// Output an entire listing (complete with labels and so on).
		void output(Output &to, Arena &arena, const Listing &listing);

		// Outputs the given virtual instruction as machine code. It must have been
		// transformed first, since transforms may remove some virtual op-codes completely.
		void output(Output &to, Arena &arena, const Frame &frame, const Instruction &from);

		//////////////////////////////////////////////////////////////////////////
		// Call examination
		//////////////////////////////////////////////////////////////////////////

		// The type used to represent a frame on the stack.
		// typedef X StackFrame

		// Type used for function metadata.
		// typedef X FnMeta

		// Information about a variable on the stack.
		struct VarInfo {
			// The location of the variable on the stack.
			void *ptr;

			// The free function for this variable, is null if the variable/parameter
			// is declared as not being destroyed on exceptions.
			void *freeFn;

			// How do we destroy this variable?
			FreeOpt freeOpt;
		};

		// Get the active block in the call frame.
		nat activeBlock(const StackFrame &frame, const FnMeta *data);

		// Get the parent stack frame.
		StackFrame parentFrame(const StackFrame &frame, const FnMeta *data);

		// Get the address of the given variable on the stack frame given.
		VarInfo variableInfo(const StackFrame &frame, const FnMeta *data, Variable variable);
		VarInfo variableInfo(const StackFrame &frame, const FnMeta *data, nat variableId);

	}

}
