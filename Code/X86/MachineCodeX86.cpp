#include "StdAfx.h"
#include "MachineCode.h"
#ifdef X86
#include "Instruction.h"
#include "AsmX86.h"
#include "Transform64.h"
#include "TransformX86.h"
#include "VariableX86.h"
#include "Listing.h"
#include "Frame.h"
#include "OpTable.h"

namespace code {

	namespace machine {
#define OUTPUT(x) { op::x, &machineX86::x }

		typedef void (*OutputFn)(Output &, machineX86::Params, const Instruction &);

		static OpEntry<OutputFn> outputMap[] = {
			OUTPUT(mov),
			OUTPUT(push),
			OUTPUT(pop),
			OUTPUT(jmp),
			OUTPUT(call),
			OUTPUT(ret),

			OUTPUT(add),
			OUTPUT(adc),
			OUTPUT(or),
			OUTPUT(and),
			OUTPUT(sub),
			OUTPUT(sbb),
			OUTPUT(xor),
			OUTPUT(cmp),

			OUTPUT(dat),

			OUTPUT(addRef),
			OUTPUT(releaseRef),

			OUTPUT(threadLocal),
		};

		static OpCode immReg[] = {
			op::mov, op::add, op::adc, op::or, op::and, op::sub, op::sbb, op::xor, op::cmp
		};

	}

	namespace machineX86 {

		State::State() : currentBlock() {}

		const Register ptrD = Register(0x100);
		const Register ptrSi = Register(0x101);
		const Register ptrDi = Register(0x102);

		const Register dl = Register(0x110);

		const Register edx = Register(0x140);
		const Register esi = Register(0x141);
		const Register edi = Register(0x142);

		const wchar_t *name(Register r) {
			switch (r) {
				case ptrD:
					return L"ptrD";
				case ptrSi:
					return L"ptrSi";
				case ptrDi:
					return L"ptrDi";

				case dl:
					return L"dl";

				case edx:
					return L"edx";
				case esi:
					return L"esi";
				case edi:
					return L"edi";
				default:
					return null;
			}
		}

		void add64(Registers &r) {
			if (r.contains(rax) && r.largest(rax) == rax) r += edx;
			if (r.contains(rbx) && r.largest(rbx) == rbx) r += esi;
			if (r.contains(rcx) && r.largest(rcx) == rcx) r += edi;
		}

		bool has64(const Listing &l) {
			for (nat i = 0; i < l.size(); i++) {
				if (l[i].size() == 8)
					return true;
			}
			return false;
		}

		Register high32(Register r) {
			r = asSize(r, 8);
			if (r == rax) return edx;
			if (r == rbx) return esi;
			if (r == rcx) return edi;
			assert(false);
			return noReg;
		}

		Register low32(Register r) {
			return asSize(r, 4);
		}

		Value high32(const Value &v) {
			assert(v.size() == 8);
			switch (v.type()) {
				case Value::tConstant:
					return natConst(v.constant() >> 32);
				case Value::tRegister:
					return Value(high32(v.reg()));
				case Value::tRelative:
					return intRel(v.reg(), v.offset() + 4);
				case Value::tVariable:
					return intRel(v.variable(), v.offset() + 4);
			}
			assert(false);
			return Value();
		}

		Value low32(const Value &v) {
			assert(v.size() == 8);
			switch (v.type()) {
				case Value::tConstant:
					return natConst(v.constant() & 0xFFFFFFFF);
				case Value::tRegister:
					return Value(low32(v.reg()));
				case Value::tRelative:
					return intRel(v.reg(), v.offset());
				case Value::tVariable:
					return intRel(v.variable(), v.offset());
			}
			assert(false);
			return Value();
		}

		nat registerId(Register r) {
			switch (r) {
				case al:
				case ptrA:
				case eax:
					return 0;
				case cl:
				case ptrC:
				case ecx:
					return 1;
				case dl:
				case ptrD:
				case edx:
					return 2;
				case bl:
				case ptrB:
				case ebx:
					return 3;
				case ptrSi:
				case esi:
					return 6;
				case ptrDi:
				case edi:
					return 7;
				
				case ptrStack:
					return 4;
				case ptrFrame:
					return 5;
				
				default:
					assert(false);
					return 0;
			}
		}

		// See if this value can be expressed in a single byte without loss.
		// Assumes that the single byte is sign-extended.
		bool singleByte(Word value) {
			Long v(value);
			return (v >= -128 && v <= 127);
		}

		// Generates a SIB-byte that express the address: baseReg + scaledReg*scale.
		// NOTE: scale must be either 1, 2, 4 or 8.
		// NOTE: baseReg == ebp does _not_ work.
		// NOTE: scaledReg == esp does _not_ work.
		// When scaledReg == -1 (0xFF) no scaling is used. (scale must be 1)
		byte sibValue(byte baseReg, byte scaledReg = -1, byte scale = 1) {
			assert(baseReg != 5);
			assert(scaledReg != 4);
			if (scaledReg == 0xFF)
				scaledReg = 4; // no scaling
			assert(scaledReg != 4 || scale == 1);
			assert(scale <= 4);
			static const byte scaleMap[9] = { -1, 0, 1, -1, 2, -1, -1, -1, 3 };
			scale = scaleMap[scale];
			assert(scale < 4 && baseReg < 8 && scaledReg < 8);
			return (scale << 6) | (scaledReg << 3) | baseReg;
		}

		byte modRmValue(byte mode, byte src, byte dest) {
			assert(mode < 4 && src < 8 && dest < 8);
			return (mode << 6) | (src << 3) | dest;
		}

		void modRm(Output &to, byte subOp, const Value &dest) {
			switch (dest.type()) {
				case Value::tRegister:
					to.putByte(modRmValue(3, subOp, registerId(dest.reg())));
					break;
				case Value::tRelative:
					if (dest.reg() == noReg) {
						to.putByte(modRmValue(0, subOp, 5));
						to.putInt(dest.offset());
					} else {
						byte mode = 2;
						nat reg = registerId(dest.reg());

						if (dest.offset() == 0) {
							if (reg == 5) {
								// We need to used disp8 for ebp...
								mode = 1;
							} else {
								mode = 0;
							}
						} else if (singleByte(dest.offset())) {
							mode = 1;
						}

						to.putByte(modRmValue(mode, subOp, reg));
						if (reg == 4) {
							// SIB-byte for reg=ESP!
							to.putByte(sibValue(reg));
						}

						if (mode == 1) {
							to.putByte(byte(dest.offset()));
						} else if (mode == 2) {
							to.putInt(cpuNat(dest.offset()));
						}
					}
					break;
				default:
					// Not implemented yet, there are many more!
					assert(false);
					break;
			}
		}

		void immRegInstr(Output &to, const ImmRegInstr &op, const Value &dest, const Value &src) {
			switch (src.type()) {
				case Value::tLabel:
					to.putByte(op.opImm32);
					modRm(to, op.modeImm32, dest);
					to.putAddress(src.label());
					break;
				case Value::tReference:
					to.putByte(op.opImm32);
					modRm(to, op.modeImm32, dest);
					to.putAddress(src.reference());
					break;
				case Value::tConstant:
					if (op.modeImm8 != 0xFF && singleByte(src.constant())) {
						to.putByte(op.opImm8);
						modRm(to, op.modeImm8, dest);
						to.putByte(src.constant() & 0xFF);
					} else {
						to.putByte(op.opImm32);
						modRm(to, op.modeImm32, dest);
						to.putInt(cpuNat(src.constant()));
					}
					break;
				case Value::tRegister:
					to.putByte(op.opSrcReg);
					modRm(to, registerId(src.reg()), dest);
					break;
				default:
					if (dest.type() == Value::tRegister) {
						to.putByte(op.opDestReg);
						modRm(to, registerId(dest.reg()), src);
					} else {
						assert(false); // This mode is _not_ supported.
					}
					break;
			}
		}

		void immRegInstr(Output &to, const ImmRegInstr8 &op, const Value &dest, const Value &src) {
			switch (src.type()) {
				case Value::tConstant:
					to.putByte(op.opImm);
					modRm(to, op.modeImm, dest);
					to.putByte(src.constant() & 0xFF);
					break;
				case Value::tRegister:
					to.putByte(op.opSrcReg);
					modRm(to, registerId(src.reg()), dest);
					break;
				default:
					if (dest.type() == Value::tRegister) {
						to.putByte(op.opDestReg);
						modRm(to, registerId(dest.reg()), src);
					} else {
						assert(false); // not supported on x86
					}
					break;
			}
		}

		void immRegInstr(Output &to, const ImmRegInstr8 &op8, const ImmRegInstr &op, const Value &dest, const Value &src) {
			nat size = src.size();
			if (size == 4) {
				immRegInstr(to, op, dest, src);
			} else if (size == 1) {
				immRegInstr(to, op8, dest, src);
			} else {
				assert(false);
			}
		}

		ImmRegTfm::ImmRegTfm(const Listing &from) : Transformer(from), registers(from) {}

		void ImmRegTfm::transform(Listing &to, nat line) {
			if (!needsImmediate(line)) {
				to << from[line];
			} else if (supported(line)) {
				to << from[line];
			} else {
				const Instruction &instr = from[line];
				nat size = instr.src().size();

				assert(size <= 4); // Not implemented for 8 byte values.

				Register reg = suitableRegister(line);

				if (reg == noReg) {
					reg = asSize(ptrD, size);
					to << code::push(ptrD);
					to << code::mov(reg, instr.src());
					to << instr.alterSrc(reg);
					to << code::pop(ptrD);
				} else {
					reg = asSize(reg, size);
					to << code::mov(reg, instr.src());
					to << instr.alterSrc(reg);
				}
			}
		}

		bool ImmRegTfm::needsImmediate(nat line) {
			for (nat i = 0; i < ARRAY_SIZE(machine::immReg); i++) {
				if (machine::immReg[i] == from[line].op())
					return true;
			}
			return false;
		}

		bool ImmRegTfm::supported(nat line) {
			switch (from[line].src().type()) {
				case Value::tLabel:
				case Value::tReference:
				case Value::tConstant:
				case Value::tRegister:
					return true;
				default:
					if (from[line].dest().type() == Value::tRegister) {
						return true;
					}
					break;
			}

			return false;
		}

		Register ImmRegTfm::suitableRegister(nat line) {
			Registers regs = registers[line];
			add64(regs);

			Register order[] = { ptrD, ptrSi, ptrDi };

			for (nat i = 0; i < ARRAY_SIZE(order); i++) {
				if (!regs.contains(order[i]))
					return order[i];
			}

			return noReg;
		}
	}

	namespace machine {

		const wchar_t *name(Register r) { return machineX86::name(r); }

		//////////////////////////////////////////////////////////////////////////
		// Defines the instruction lookup.
		//////////////////////////////////////////////////////////////////////////

		Listing transform(const Listing &from, const Binary *owner) {
			Listing middle;

			const Listing *src = &from;
			if (machineX86::has64(from)) {
				machineX86::Transform64 tfm(from);
				middle = tfm.transformed();
				src = &middle;
			}

			machineX86::ImmRegTfm tfm(*src);
			middle = tfm.transformed();

			machineX86::TfmParams params(middle, owner);
			return params.transformed();
		}

		void output(Output &to, Arena &arena, const Frame &frame, State &state, const Instruction &from) {
			static OpTable<OutputFn> outputs(outputMap, ARRAY_SIZE(outputMap));

			OutputFn output = outputs[from.op()];
			assert(output); // Possibly forgotten transform
			if (!output)
				return;

			machineX86::Params params = { arena, frame, state };
			(*output)(to, params, from);
		}

	}
}

#endif