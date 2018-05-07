#include "stdafx.h"
#include "WeakCast.h"
#include "Named.h"
#include "Compiler/Exception.h"
#include "Compiler/Engine.h"
#include "Compiler/Lib/Maybe.h"

namespace storm {
	namespace bs {

		WeakCast::WeakCast(SrcPos pos) : pos(pos) {}

		MAYBE(Str *) WeakCast::overwrite() {
			return null;
		}

		MAYBE(Str *) WeakCast::defaultOverwrite(Expr *expr) {
			if (LocalVarAccess *var = as<LocalVarAccess>(expr))
				return new (this) Str(*var->var->name);

			// TODO? Enforce 'this' is used as the parameter?
			if (MemberVarAccess *var = as<MemberVarAccess>(expr))
				return new (this) Str(*var->var->name);

			return null;
		}

		Value WeakCast::result() {
			return Value();
		}

		void WeakCast::code(CodeGen *state, CodeResult *boolResult, MAYBE(LocalVar *) var) {
			using namespace code;

			// Always indicate cast failed.
			*state->l << mov(boolResult->location(state).v, byteConst(0));
		}


		/**
		 * 'as' cast.
		 */

		WeakDowncast::WeakDowncast(Block *block, Expr *expr, SrcName *type)
			: WeakCast(expr->pos), expr(expr) {

			to = block->scope.value(type);

			Value from = expr->result().type();
			if (isMaybe(from))
				from = unwrapMaybe(from);

			if (!from.isHeapObj())
				throw SyntaxError(expr->pos, L"The 'as' operator is only applicable to class or actor types.");
			if (!to.type->isA(from.type))
				throw SyntaxError(expr->pos, L"Condition is always false. " + ::toS(to) +
								L" does not inherit from " + ::toS(from) + L".");
		}

		MAYBE(Str *) WeakDowncast::overwrite() {
			return defaultOverwrite(expr);
		}

		Value WeakDowncast::result() {
			return to;
		}

		void WeakDowncast::code(CodeGen *state, CodeResult *boolResult, MAYBE(LocalVar *) var) {
			using namespace code;

			// Get the value...
			Value fromType = expr->result().type();
			CodeResult *from = new (this) CodeResult(fromType, state->block);
			expr->code(state, from);

			// Load into eax...
			if (fromType.ref) {
				*state->l << mov(ptrA, from->location(state).v);
				*state->l << mov(ptrA, ptrRel(ptrA, Offset()));
			} else {
				*state->l << mov(ptrA, from->location(state).v);
			}

			// Store the object into 'var' regardless if the test succeeded. It does not matter.
			if (var) {
				*state->l << mov(var->var.v, ptrA);
				var->var.created(state);
			}

			// Call the 'as' function.
			*state->l << fnParam(engine().ptrDesc(), ptrA);
			*state->l << fnParam(engine().ptrDesc(), to.type->typeRef());
			*state->l << fnCall(engine().ref(Engine::rAs), false, engine().ptrDesc(), ptrA);
			*state->l << cmp(ptrA, ptrConst(Offset()));
			*state->l << setCond(al, ifNotEqual);
			*state->l << mov(boolResult->location(state).v, al);
		}

		void WeakDowncast::toS(StrBuf *to) const {
			*to << expr << S(" as ") << this->to;
		}

		/**
		 * MaybeCast.
		 */

		WeakMaybeCast::WeakMaybeCast(Expr *expr) : WeakCast(expr->pos), expr(expr) {}

		MAYBE(Str *) WeakMaybeCast::overwrite() {
			return defaultOverwrite(expr);
		}

		Value WeakMaybeCast::result() {
			return unwrapMaybe(expr->result().type());
		}

		void WeakMaybeCast::code(CodeGen *state, CodeResult *boolResult, MAYBE(LocalVar *) var) {
			using namespace code;

			Value srcType = expr->result().type();
			if (MaybeClassType *c = as<MaybeClassType>(srcType.type)) {
				classCode(c, state, boolResult, var);
			} else if (MaybeValueType *v = as<MaybeValueType>(srcType.type)) {
				valueCode(v, state, boolResult, var);
			}
		}

		void WeakMaybeCast::classCode(MaybeClassType *c, CodeGen *state, CodeResult *boolResult, MAYBE(LocalVar *) var) {
			using namespace code;

			CodeResult *from;
			if (var)
				from = new (this) CodeResult(Value(c), var->var);
			else
				from = new (this) CodeResult(Value(c), state->block);
			expr->code(state, from);

			// Check if it is null.
			*state->l << cmp(from->location(state).v, ptrConst(Offset()));
			*state->l << setCond(boolResult->location(state).v, ifNotEqual);
		}

		void WeakMaybeCast::valueCode(MaybeValueType *c, CodeGen *state, CodeResult *boolResult, MAYBE(LocalVar *) var) {
			using namespace code;

			CodeResult *from = new (this) CodeResult(Value(c), state->block);
			expr->code(state, from);
			from->location(state).created(state);

			Label end = state->l->label();

			// Check if it is null.
			*state->l << lea(ptrC, from->location(state).v);
			*state->l << cmp(byteRel(ptrC, c->boolOffset()), byteConst(0));
			*state->l << setCond(boolResult->location(state).v, ifNotEqual);
			*state->l << jmp(end, ifEqual);

			// Copy it if we want a result, and if we weren't null.
			if (var) {
				*state->l << lea(ptrA, var->var.v);
				if (c->param().isBuiltIn()) {
					Size sz = c->param().size();
					*state->l << mov(xRel(sz, ptrA, Offset()), xRel(sz, ptrC, Offset()));
				} else {
					*state->l << fnParam(engine().ptrDesc(), ptrA);
					*state->l << fnParam(engine().ptrDesc(), ptrC);
					*state->l << fnCall(c->param().copyCtor(), true);
				}
			}

			*state->l << end;
		}

		void WeakMaybeCast::toS(StrBuf *to) const {
			*to << expr;
		}


		/**
		 * Helpers.
		 */

		WeakCast *weakAsCast(Block *block, Expr *expr, SrcName *type) {
			// Currently, only downcast is supported.
			return new (expr) WeakDowncast(block, expr, type);
		}


	}
}
