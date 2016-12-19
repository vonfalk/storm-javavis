#include "stdafx.h"
#include "Expr.h"
#include "Core/Str.h"
#include "Exception.h"

namespace storm {
	namespace bs {

		Expr::Expr(SrcPos pos) : pos(pos) {}

		ExprResult Expr::result() {
			return ExprResult();
		}

		void Expr::code(CodeGen *to, CodeResult *var) {
			assert(!var->needed());
		}

		Int Expr::castPenalty(Value to) {
			return -1;
		}

		Constant::Constant(SrcPos pos, Int v) : Expr(pos), cType(tInt), intValue(v) {}

		Constant::Constant(SrcPos pos, Long v) : Expr(pos), cType(tInt), intValue(v) {}

		Constant::Constant(SrcPos pos, Float v) : Expr(pos), cType(tFloat), floatValue(v) {}

		Constant::Constant(SrcPos pos, Str *v) : Expr(pos), cType(tStr), strValue(v) {}

		Constant::Constant(SrcPos pos, Bool v) : Expr(pos), cType(tBool), boolValue(v) {}

		void Constant::toS(StrBuf *to) const {
			switch (cType) {
			case tInt:
				*to << intValue << L"i";
				break;
			case tFloat:
				*to << floatValue << L"f";
				break;
			case tStr:
				*to << L"\"" << strValue << L"\"";
				break;
			case tBool:
				*to << (boolValue ? L"true" : L"false");
				break;
			default:
				*to << L"UNKNOWN";
				break;
			}
		}

		ExprResult Constant::result() {
			switch (cType) {
			case tInt:
				return Value(StormInfo<Int>::type(engine()));
			case tFloat:
				return Value(StormInfo<Float>::type(engine()));
			case tStr:
				return Value(StormInfo<Str>::type(engine()));
			case tBool:
				return Value(StormInfo<Bool>::type(engine()));
			default:
				TODO("Implement missing type");
				return Value();
			}
		}

		Int Constant::castPenalty(Value to) {
			if (cType != tInt)
				return -1;

			if (to.ref)
				return -1;

			// Prefer bigger types if multiple are possible.
			Engine &e = engine();
			if (to.type == StormInfo<Long>::type(e))
				return 1;
			if (to.type == StormInfo<Word>::type(e))
				return 1;
			if (to.type == StormInfo<Int>::type(e) && (abs(intValue) & 0x7FFFFFFF) == abs(intValue))
				return 2;
			if (to.type == StormInfo<Nat>::type(e) && (intValue & 0xFFFFFFFF) == intValue)
				return 2;
			if (to.type == StormInfo<Byte>::type(e) && (intValue & 0xFF) == intValue)
				return 3;
			if (to.type == StormInfo<Float>::type(e) && (abs(intValue) & 0xFFFF) == abs(intValue))
				// We allow up to 16 bits to automatically cast.
				return 3;

			return -1;
		}

		void Constant::code(CodeGen *s, CodeResult *r) {
			using namespace code;

			if (!r->needed())
				return;

			switch (cType) {
			case tInt:
				intCode(s, r);
				break;
			case tFloat:
				floatCode(s, r);
				break;
			case tStr:
				strCode(s, r);
				break;
			case tBool:
				boolCode(s, r);
				break;
			default:
				TODO("Implement missing type");
				break;
			}
		}

		void Constant::strCode(CodeGen *s, CodeResult *r) {
			using namespace code;

			// We can store the string as an object inside the code.
			VarInfo to = r->location(s);
			*s->to << mov(to.v, objPtr(strValue));
		}

		void Constant::intCode(CodeGen *s, CodeResult *r) {
			using namespace code;

			VarInfo to = r->location(s);

			Type *t = r->type().type;
			Engine &e = engine();
			if (t == StormInfo<Int>::type(e))
				*s->to << mov(to.v, intConst(int(intValue)));
			else if (t == StormInfo<Nat>::type(e))
				*s->to << mov(to.v, natConst(nat(intValue)));
			else if (t == StormInfo<Byte>::type(e))
				*s->to << mov(to.v, byteConst(byte(intValue)));
			else if (t == StormInfo<Float>::type(e))
				*s->to << mov(to.v, floatConst(float(intValue)));
			else if (t == StormInfo<Long>::type(e))
				*s->to << mov(to.v, longConst(Long(intValue)));
			else if (t == StormInfo<Word>::type(e))
				*s->to << mov(to.v, wordConst(Word(intValue)));
			else
				assert(false, L"Unknown type for an integer constant.");

			to.created(s);
		}

		void Constant::floatCode(CodeGen *s, CodeResult *r) {
			using namespace code;

			VarInfo to = r->location(s);
			Type *t = r->type().type;
			Engine &e = engine();
			if (t != StormInfo<Float>::type(e))
				assert(false, L"Unknown type for a float constant.");

			*s->to << mov(to.v, floatConst(float(floatValue)));

			to.created(s);
		}

		void Constant::boolCode(CodeGen *s, CodeResult *r) {
			using namespace code;

			VarInfo to = r->location(s);
			*s->to << mov(to.v, byteConst(boolValue ? 1 : 0));
			to.created(s);
		}

		Constant *intConstant(SrcPos pos, Str *v) {
			return CREATE(Constant, v->engine(), pos, v->toLong());
		}

		Constant *floatConstant(SrcPos pos, Str *v) {
			return CREATE(Constant, v->engine(), pos, v->toFloat());
		}

		Constant *strConstant(SrcPos pos, Str *v) {
			return CREATE(Constant, v->engine(), pos, v->unescape());
		}

		Constant *rawStrConstant(SrcPos pos, Str *v) {
			return CREATE(Constant, v->engine(), pos, v);
		}

		Constant *trueConstant(EnginePtr e, SrcPos pos) {
			return CREATE(Constant, e.v, pos, true);
		}

		Constant *falseConstant(EnginePtr e, SrcPos pos) {
			return CREATE(Constant, e.v, pos, false);
		}

		/**
		 * Dummy expression.
		 */

		DummyExpr::DummyExpr(SrcPos pos, Value type) : Expr(pos), type(type) {}

		ExprResult DummyExpr::result() {
			return type;
		}

		void DummyExpr::code(CodeGen *s, CodeResult *r) {
			throw InternalError(L"Tried to generate code from a DummyExpr!");
		}


	}
}
