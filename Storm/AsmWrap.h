#pragma once
#include "Code/Listing.h"
#include "Code/Instruction.h"
#include "Lib/Object.h"

namespace storm {

	namespace wrap {

		/**
		 * TODO: Create toS or similar functions for these types!
		 */


		// Wrapper for the code listing.
		STORM_PKG(core.asm);

		// Size.
		class Size {
			STORM_VALUE;
		public:
			STORM_CTOR Size();

			// Convert from the type in code.
			Size(const code::Size &v);

			// Value.
			code::Size v;

			// Operators.
			Bool STORM_FN operator ==(const Size &o) const;
			Size STORM_FN operator +(const Size &o) const;
			Size &STORM_FN operator +=(const Size &o);
		};

		// Get some pre-defined sizes.
		Size STORM_FN sizeChar();
		Size STORM_FN sizeByte();
		Size STORM_FN sizeInt();
		Size STORM_FN sizeNat();
		Size STORM_FN sizeLong();
		Size STORM_FN sizeWord();
		Size STORM_FN sizePtr();

		// Offset.
		class Offset {
			STORM_VALUE;
		public:
			STORM_CTOR Offset();

			// Convert from the type in code.
			Offset(const code::Offset &v);

			// Value.
			code::Offset v;

			// Operators.
			Bool STORM_FN operator ==(const Offset &o) const;
			Offset STORM_FN operator +(const Offset &o) const;
			Offset &STORM_FN operator +=(const Offset &o);
		};

		// Get some pre-defined offsets.
		Offset STORM_FN offsetChar();
		Offset STORM_FN offsetByte();
		Offset STORM_FN offsetInt();
		Offset STORM_FN offsetNat();
		Offset STORM_FN offsetLong();
		Offset STORM_FN offsetWord();
		Offset STORM_FN offsetPtr();

		// Variable.
		class Variable {
			STORM_VALUE;
		public:
			inline STORM_CTOR Variable() {}
			inline Variable(code::Variable v) : v(v) {}
			code::Variable v;
		};

		// Block.
		class Block {
			STORM_VALUE;
		public:
			inline STORM_CTOR Block() {}
			inline Block(code::Block v) : v(v) {}
			code::Block v;
		};

		// Part.
		class Part {
			STORM_VALUE;
		public:
			inline STORM_CTOR Part() {}
			inline STORM_CTOR Part(Block b) : v(b.v) {}
			inline Part(code::Part v) : v(v) {}
			code::Part v;
		};

		// Wrapper for the Register type.
		class Register {
			STORM_VALUE;
		public:
			STORM_CTOR Register();
			inline Register(code::Register v) : v(v) {}
			code::Register v;
		};

		// Create registers.
		Register STORM_FN ptrStack();
		Register STORM_FN ptrFrame();
		Register STORM_FN ptrA();
		Register STORM_FN ptrB();
		Register STORM_FN ptrC();
		Register STORM_FN al();
		Register STORM_FN bl();
		Register STORM_FN cl();
		Register STORM_FN eax();
		Register STORM_FN ebx();
		Register STORM_FN ecx();
		Register STORM_FN rax();
		Register STORM_FN rbx();
		Register STORM_FN rcx();

		// Wrapper for CondFlag.
		class CondFlag {
			STORM_VALUE;
		public:
			inline CondFlag(code::CondFlag flag) : v(flag) {}
			code::CondFlag v;

			// Invert it.
			CondFlag STORM_FN inverse() const;
		};

		// Create.
		CondFlag STORM_FN ifAlways();
		CondFlag STORM_FN ifNever();
		CondFlag STORM_FN ifOverflow();
		CondFlag STORM_FN ifNoOverflow();
		CondFlag STORM_FN ifEqual();
		CondFlag STORM_FN ifNotEqual();
		CondFlag STORM_FN ifBelow();
		CondFlag STORM_FN ifBelowEqual();
		CondFlag STORM_FN ifAboveEqual();
		CondFlag STORM_FN ifAbove();
		CondFlag STORM_FN ifLess();
		CondFlag STORM_FN ifLessEqual();
		CondFlag STORM_FN ifGreaterEqual();
		CondFlag STORM_FN ifGreater();


		// Operand (alias for Value in Code, to avoid ambiguous names).
		class Operand {
			STORM_VALUE;
		public:
			// Create the invalid operand.
			STORM_CTOR Operand();

			// Register.
			STORM_CTOR Operand(Register r);

			// CondFlag.
			STORM_CTOR Operand(CondFlag flag);

			// Variable.
			STORM_CTOR Operand(Variable v);

			// Part.
			STORM_CTOR Operand(Part p);

			// Block.
			STORM_CTOR Operand(Block b);

			// Convert.
			Operand(const code::Value &v);

			// Value.
			code::Value v;

			// Equals.
			Bool STORM_FN operator ==(const Operand &o) const;

			// Size.
			Size STORM_FN size() const;
		};

		// Create. (some types does not yet exist in Storm).
		// See Code/Value.h for documentation.
		Operand STORM_FN byteConst(Byte v);
		Operand STORM_FN intConst(Int v);
		Operand STORM_FN natConst(Nat v);
		Operand STORM_FN natConst(Size v);
		Operand STORM_FN intPtrConst(Int v);
		Operand STORM_FN natPtrConst(Nat v);
		Operand STORM_FN ptrConst(Size v);
		Operand STORM_FN byteRel(Register reg, Offset offset);
		Operand STORM_FN intRel(Register reg, Offset offset);
		Operand STORM_FN longRel(Register reg, Offset offset);
		Operand STORM_FN ptrRel(Register reg, Offset offset);
		Operand STORM_FN xRel(Size size, Register reg, Offset offset);
		Operand STORM_FN byteRel(Variable v, Offset offset);
		Operand STORM_FN intRel(Variable v, Offset offset);
		Operand STORM_FN longRel(Variable v, Offset offset);
		Operand STORM_FN ptrRel(Variable v, Offset offset);
		Operand STORM_FN xRel(Size size, Variable v, Offset offset);

		// Instruction.
		class Instruction {
			STORM_VALUE;
		public:
			Instruction(const code::Instruction &v);

			// Value.
			code::Instruction v;
		};

		// Create instructions.
		Instruction STORM_FN prolog();
		Instruction STORM_FN epilog();
		Instruction STORM_FN ret(Size s);

		Instruction STORM_FN addRef(Operand v);
		Instruction STORM_FN releaseRef(Operand v);

		Instruction STORM_FN mov(Operand to, Operand from);

		// FreeOptions
		class FreeOn {
			STORM_VALUE;
		public:
			inline STORM_CTOR FreeOn() : v(code::freeDef) {}
			inline FreeOn(code::FreeOpt v) : v(v) {}
			code::FreeOpt v;

			// Bitwise or.
			inline FreeOn operator |(const FreeOn o) const { return v | o.v; }
		};

		// Create.
		FreeOn STORM_FN freeOnNone();
		FreeOn STORM_FN freeOnException();
		FreeOn STORM_FN freeOnBlockExit();
		FreeOn STORM_FN freeOnBoth();
		FreeOn STORM_FN freePtr();


		// Listing
		class Listing : public Object {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR Listing();
			STORM_CTOR Listing(Par<Listing> o);

			// Data.
			code::Listing v;

			// The functionality of the frame is integrated here as well. This may or may not be a good idea.
			Block STORM_FN root();
			Block STORM_FN createChild(Part parent);
			Part STORM_FN createPart(Part after);
			void STORM_FN delay(Variable v, Part to);

			Variable STORM_FN createByteVar(Part in, Operand free, FreeOn on);
			Variable STORM_FN createIntVar(Part in, Operand free, FreeOn on);
			Variable STORM_FN createLongVar(Part in, Operand free, FreeOn on);
			Variable STORM_FN createPtrVar(Part in, Operand free, FreeOn on);
			Variable STORM_FN createVariable(Part in, Size size, Operand free, FreeOn on);

			Variable STORM_FN createByteParam(Operand free, FreeOn on);
			Variable STORM_FN createIntParam(Operand free, FreeOn on);
			Variable STORM_FN createLongParam(Operand free, FreeOn on);
			Variable STORM_FN createPtrParam(Operand free, FreeOn on);
			Variable STORM_FN createParameter(Size size, Bool isFloat, Operand free, FreeOn on);

			// Append instructions.
			Listing *STORM_FN operator <<(const Instruction &v);

		protected:
			// Output.
			virtual void output(wostream &to) const;
		};

	}
}
