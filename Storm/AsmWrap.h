#pragma once
#include "Code/Listing.h"
#include "Code/Instruction.h"
#include "Shared/EnginePtr.h"
#include "Lib/Object.h"

namespace storm {

	namespace wrap {

		// Wrapper for the code listing.
		STORM_PKG(core.asm);

		// Size.
		class Size {
			STORM_VALUE;
		public:
			STORM_CTOR Size();

			// Prefer constructing from the sizeXxx constants below.
			STORM_CTOR Size(Nat size);

			// Convert from the type in code.
			Size(const code::Size &v);

			// Value.
			code::Size v;

			// Operators.
			Bool STORM_FN operator ==(const Size &o) const;
			Size STORM_FN operator +(const Size &o) const;
			Size &STORM_FN operator +=(const Size &o);

			// Deep copy.
			inline void STORM_FN deepCopy(Par<CloneEnv> e) {}
		};

		wostream &operator <<(wostream &to, const Size &s);
		Str *STORM_ENGINE_FN toS(EnginePtr e, Size s);

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

			// Deep copy.
			inline void STORM_FN deepCopy(Par<CloneEnv> e) {}
		};

		wostream &operator <<(wostream &to, const Offset &s);
		Str *STORM_ENGINE_FN toS(EnginePtr e, Offset s);

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

			// Deep copy.
			inline void STORM_FN deepCopy(Par<CloneEnv> e) {}
		};

		wostream &operator <<(wostream &to, const Variable &s);
		Str *STORM_ENGINE_FN toS(EnginePtr e, Variable s);

		// Block.
		class Block {
			STORM_VALUE;
		public:
			inline STORM_CTOR Block() {}
			inline Block(code::Block v) : v(v) {}
			code::Block v;

			// Deep copy.
			inline void STORM_FN deepCopy(Par<CloneEnv> e) {}
		};

		wostream &operator <<(wostream &to, const Block &s);
		Str *STORM_ENGINE_FN toS(EnginePtr e, Block s);

		// Part.
		class Part {
			STORM_VALUE;
		public:
			inline STORM_CTOR Part() {}
			inline STORM_CAST_CTOR Part(Block b) : v(b.v) {}
			inline Part(code::Part v) : v(v) {}
			code::Part v;

			// Deep copy.
			inline void STORM_FN deepCopy(Par<CloneEnv> e) {}
		};

		wostream &operator <<(wostream &to, const Part &s);
		Str *STORM_ENGINE_FN toS(EnginePtr e, Part s);


		// Label.
		class Label {
			STORM_VALUE;
		public:
			inline Label(code::Label v) : v(v) {}
			code::Label v;

			// Deep copy.
			inline void STORM_FN deepCopy(Par<CloneEnv> e) {}
		};

		wostream &operator <<(wostream &to, const Label &s);
		Str *STORM_ENGINE_FN toS(EnginePtr e, Label s);

		// Wrapper for the Register type.
		class Register {
			STORM_VALUE;
		public:
			STORM_CTOR Register();
			inline Register(code::Register v) : v(v) {}
			code::Register v;

			// Deep copy.
			inline void STORM_FN deepCopy(Par<CloneEnv> e) {}
		};

		wostream &operator <<(wostream &to, const Register &s);
		Str *STORM_ENGINE_FN toS(EnginePtr e, Register s);

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

			// Deep copy.
			inline void STORM_FN deepCopy(Par<CloneEnv> e) {}
		};

		wostream &operator <<(wostream &to, const CondFlag &s);
		Str *STORM_ENGINE_FN toS(EnginePtr e, CondFlag s);

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

			// Variable.
			STORM_CTOR Operand(Label l);

			// Convert.
			Operand(const code::Value &v);

			// Value.
			code::Value v;

			// Equals.
			Bool STORM_FN operator ==(const Operand &o) const;

			// Size.
			Size STORM_FN size() const;

			// Deep copy.
			inline void STORM_FN deepCopy(Par<CloneEnv> e) {}
		};

		wostream &operator <<(wostream &to, const Operand &o);
		Str *STORM_ENGINE_FN toS(EnginePtr e, Operand o);

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

		// RetVal.
		class RetVal {
			STORM_VALUE;
		public:
			inline STORM_CTOR RetVal(Size s, Bool isFloat) : size(s), isFloat(isFloat) {}

			STORM_VAR Size size;
			STORM_VAR Bool isFloat;

			inline code::RetVal v() { return code::retVal(size.v, isFloat); }
		};

		wostream &operator <<(wostream &to, const RetVal &s);
		Str *STORM_ENGINE_FN toS(EnginePtr e, RetVal s);

		// Instruction.
		class Instruction {
			STORM_VALUE;
		public:
			Instruction(const code::Instruction &v);

			// Value.
			code::Instruction v;

			// Deep copy.
			inline void STORM_FN deepCopy(Par<CloneEnv> e) {}
		};

		wostream &operator <<(wostream &to, const Instruction &s);
		Str *STORM_ENGINE_FN toS(EnginePtr e, Instruction s);

		// Create instructions.
		Instruction STORM_FN prolog();
		Instruction STORM_FN epilog();
		Instruction STORM_FN call(Operand to, RetVal r);
		Instruction STORM_FN ret(RetVal s);
		Instruction STORM_FN jmp(Operand to);
		Instruction STORM_FN jmp(Operand to, CondFlag cond);

		Instruction STORM_FN fnCall(Operand to, RetVal ret);
		Instruction STORM_FN fnParam(Operand param);
		Instruction STORM_FN fnParam(Variable param, Operand copy);

		Instruction STORM_FN addRef(Operand v);
		Instruction STORM_FN releaseRef(Operand v);

		Instruction STORM_FN mov(Operand to, Operand from);
		Instruction STORM_FN lea(Operand to, Operand from);

		Instruction STORM_FN push(Operand from);
		Instruction STORM_FN pop(Operand to);

		Instruction STORM_FN add(Operand dest, Operand src);
		Instruction STORM_FN adc(Operand dest, Operand src);
		Instruction STORM_FN or(Operand dest, Operand src);
		Instruction STORM_FN and(Operand dest, Operand src);
		Instruction STORM_FN sub(Operand dest, Operand src);
		Instruction STORM_FN sbb(Operand dest, Operand src);
		Instruction STORM_FN xor(Operand dest, Operand src);
		Instruction STORM_FN cmp(Operand dest, Operand src);
		Instruction STORM_FN mul(Operand dest, Operand src);


		// FreeOptions
		class FreeOn {
			STORM_VALUE;
		public:
			inline STORM_CTOR FreeOn() : v(code::freeDef) {}
			inline FreeOn(code::FreeOpt v) : v(v) {}
			code::FreeOpt v;

			// Bitwise or.
			inline FreeOn operator |(const FreeOn o) const { return v | o.v; }

			// Deep copy.
			inline void STORM_FN deepCopy(Par<CloneEnv> e) {}
		};

		wostream &operator <<(wostream &to, const FreeOn &s);
		Str *STORM_ENGINE_FN toS(EnginePtr e, FreeOn s);

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

			// Create a label.
			Label STORM_FN label();

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

			// Append instructions and labels.
			Listing *STORM_FN operator <<(const Instruction &v);
			Listing *STORM_FN operator <<(const Label &l);

		protected:
			// Output.
			virtual void output(wostream &to) const;
		};

	}
}
