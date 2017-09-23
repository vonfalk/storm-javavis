#pragma once
#include "../TypeDesc.h"
#include "../Reg.h"
#include "Core/Object.h"
#include "Core/Array.h"

namespace code {
	namespace x64 {
		STORM_PKG(core.asm.x64);

		/**
		 * Implements the logic for laying out parameters on the stack and into registers.
		 *
		 * On X86-64 parameters are passed according to the following rules:
		 * - The first six integer or pointer arguments are passed in registers
		 *   RDI, RSI, RDX, RCX, R8 and R9.
		 * - The first eight floating-point arguments are passed in registers
		 *   XMM0, XMM1, XMM2, XMM3, XMM4, XMM5, XMM6 and XMM7
		 * - Any parameters that do not fit in registers are passed on the stack.
		 * - Classes containing a trivial copy constructor are decomposed into primitive types
		 *   and passed as separate parameters. However, if the entire class does not fit into
		 *   registers, the entire class is passed on the stack.
		 * - Classes without a trivial copy constructor are passed by pointer. The caller is
		 *   responsible for copying the parameter and destroying it when the function call is
		 *   completed.
		 */


		/**
		 * Describes what a single register is supposed to contain.
		 */
		class Param {
			STORM_VALUE;
		public:
			// Create an empty parameter.
			STORM_CTOR Param();
			STORM_CTOR Param(Nat id, Primitive p);
			STORM_CTOR Param(Nat id, Nat size, Nat offset);

			// Reset.
			void clear();

			// Set to values.
			void set(Nat id, Nat size, Nat offset);

			// Get the different fields stored in here:
			inline Nat size() const {
				return data & 0xF;
			}
			inline Nat id() const {
				return (data & 0xFF0) >> 4;
			}
			inline Nat offset() const {
				return (data & 0xFFFFF000) >> 12;
			}

			// Compare for equality.
			inline Bool STORM_FN operator ==(Param o) const {
				return data == o.data;
			}
			inline Bool STORM_FN operator !=(Param o) const {
				return data != o.data;
			}

			// ID usable for a hidden return parameter.
			enum { returnId = 0xFF };

		private:
			// Stored as follows:
			// Bits 0-3: Size of the register (in bytes).
			// Bits 4-11: Parameter number. 0xFF = unused.
			// Bits 12-31: Offset into parameter (in bytes).
			Nat data;
		};

		// Output.
		wostream &operator <<(wostream &to, Param p);
		StrBuf &STORM_FN operator <<(StrBuf &to, Param p);


		/**
		 * Describes how parameters are laid out during a function call. This is specific to X86-64,
		 * so any sizes and offsets stored here are for that specific platform.
		 */
		class Params : public storm::Object {
			STORM_CLASS;
		public:
			// Create an empty parameter layout.
			STORM_CTOR Params();

			// Add a single parameter to here.
			void STORM_FN add(Nat id, TypeDesc *type);

			// Add a primitive.
			void STORM_FN add(Nat id, Primitive p);

			// To string.
			virtual void STORM_FN toS(StrBuf *to) const;

			/**
			 * Access to the stack layout.
			 */

			// Get the total number of elements on the stack.
			Nat STORM_FN stackCount() const {
				return stack->count();
			}

			// Get stack element number 'n'.
			Nat STORM_FN stackAt(Nat n) const {
				return stack->at(n);
			}

			/**
			 * Access to the registers.
			 */

			// Total number of registers to examine.
			Nat STORM_FN registerCount() const;

			// Get a parameter. Might be empty.
			Param STORM_FN registerAt(Nat n) const;

			// Get the register containing the parameter.
			Reg STORM_FN registerSrc(Nat n) const;

		private:
			// Available registers:

			// Regular integer registers.
			GcArray<Param> *integer;
			// Real registers, for floating point parameters.
			GcArray<Param> *real;
			// Stores all parameters that are to be passed on the stack.
			Array<Nat> *stack;

			// Add complex and simple structs.
			void addDesc(Nat id, ComplexDesc *type);
			void addDesc(Nat id, SimpleDesc *type);

			// Try to add a parameter to 'to', otherwise add it to the stack.
			void tryAdd(GcArray<Param> *to, Param p);

			// Try to add a parameter to a register described by 'kind'. Otherwise, do nothing.
			bool tryAdd(primitive::Kind kind, Param p);
		};

		// Create a 'params' object from a list of TypeDesc objects.
		Params *STORM_FN params(Array<TypeDesc *> *types);

		/**
		 * Describes how the return value is stored.
		 */
		class Result : public storm::Object {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR Result(TypeDesc *type);

			// We're using a maximum of 2 registers. Reg1 is always at offset 0 and reg2 at offset 8.
			primitive::PrimitiveKind part1;
			primitive::PrimitiveKind part2;

			// Return in memory?
			Bool memory;

			// To string.
			virtual void STORM_FN toS(StrBuf *to) const;

		private:
			// Helper functions.
			void add(PrimitiveDesc *desc);
			void add(ComplexDesc *desc);
			void add(SimpleDesc *desc);
		};

		// Create a 'result' object describing how the return value shall be represented.
		Result *STORM_FN result(TypeDesc *type);

	}
}
