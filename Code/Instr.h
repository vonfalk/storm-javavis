#pragma once
#include "OpCode.h"
#include "Operand.h"
#include "Core/Object.h"
#include "Core/EnginePtr.h"
#include "Utils/Bitmask.h"

namespace code {
	STORM_PKG(core.asm);

	/**
	 * Value representing an entire asm-instruction.
	 *
	 * Note: immutable class.
	 */
	class Instr : public Object {
		STORM_CLASS;
	public:
		// Create a no-op instruction.
		STORM_CTOR Instr();

		// Copy this instruction.
		STORM_CTOR Instr(const Instr &o);

		// Deep copy.
		virtual void STORM_FN deepCopy(CloneEnv *env);

		// Access to the members:
		op::Code STORM_FN op() const;
		Operand STORM_FN src() const;
		Operand STORM_FN dest() const;
		DestMode STORM_FN mode() const;

		// Get the maximum size of the operands.
		Size STORM_FN size() const;

		// Create another instruction based off this one. Intended to be used by backends -> no sanity checking.
		Instr *STORM_FN alter(Operand dest, Operand src);
		Instr *STORM_FN alterSrc(Operand src);
		Instr *STORM_FN alterDest(Operand dest);

		// To string.
		virtual void STORM_FN toS(StrBuf *to) const;

	private:
		// Private constructor, used by 'instrXxx()' functions below.
		Instr(op::Code opCode, const Operand &dest, const Operand &src);

		// Op-code.
		op::Code iOp;

		// Src and dest fields.
		Operand iSrc;
		Operand iDest;

		// Creators.
		friend Instr *instr(EnginePtr e, op::Code op);
		friend Instr *instrSrc(EnginePtr e, op::Code op, Operand src);
		friend Instr *instrDest(EnginePtr e, op::Code op, Operand dest);
		friend Instr *instrDestSrc(EnginePtr e, op::Code op, Operand dest, Operand src);
		friend Instr *instrLoose(EnginePtr e, op::Code op, Operand dest, Operand src);
	};

	// Create an instruction without operands.
	Instr *STORM_FN instr(EnginePtr e, op::Code op);
	Instr *STORM_FN instrSrc(EnginePtr e, op::Code op, Operand src);
	Instr *STORM_FN instrDest(EnginePtr e, op::Code op, Operand dest);
	Instr *STORM_FN instrDestSrc(EnginePtr e, op::Code op, Operand dest, Operand src);

	// Create without checking the sanity of the parameters. Used for pseudo-instructions and predicates among others.
	Instr *STORM_FN instrLoose(EnginePtr e, op::Code op, Operand dest, Operand src);

}
