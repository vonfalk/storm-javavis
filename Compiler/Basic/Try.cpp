#include "stdafx.h"
#include "Try.h"
#include "Cast.h"
#include "Compiler/Exception.h"
#include "Compiler/Type.h"

namespace storm {
	namespace bs {

		/**
		 * Try
		 */

		TryBlock::TryBlock(SrcPos pos, Block *parent) : ExprBlock(pos, parent) {
			toCatch = new (this) Array<CatchBlock *>();
		}

		void TryBlock::addCatch(CatchBlock *block) {
			// Make sure no mistakes were made.
			if (block->parent() == this)
				throw new (this) InternalError(S("Incorrect scope of supplied catch-block."));
			toCatch->push(block);
		}

		void TryBlock::code(CodeGen *state, CodeResult *to) {
			// Create an outer block where we can store the exception temporarily while entering
			// catch blocks (registers are not preserved during "begin").

			CodeGen *child = state->child();
			*state->l << begin(child->block);

			// Tell all catch handlers about our temporary variable where we store the exception.
			code::Var tempVar = child->l->createVar(child->block, Size::sPtr);
			for (Nat i = 0; i < toCatch->count(); i++) {
				toCatch->at(i)->exceptionVar = tempVar;
			}

			ExprBlock::code(child, to);

			*state->l << end(child->block);
		}

		void TryBlock::blockCode(CodeGen *state, CodeResult *to, code::Block block) {
			using namespace code;

			if (toCatch->empty())
				throw new (this) SyntaxError(pos, S("No catch handlers for the try-block."));

			*state->l << begin(block);
			CodeGen *child = state->child(block);
			ExprBlock::blockCode(child, to);
			*state->l << end(block);

			// Skip the others if we made it this far.
			Label end = state->l->label();
			*state->l << jmp(end);

			Value result = this->result().type();

			// All catch handlers.
			for (Nat i = 0; i < toCatch->count(); i++) {
				CatchBlock *c = toCatch->at(i);
				Label start = state->l->label();
				state->l->addCatch(block, c->type, start);

				*state->l << start;
				// Store the exception so that the block can safely call 'enterBlock'.
				*state->l << mov(c->exceptionVar, ptrA);
				expectCastTo(c, result, scope)->code(state, to);

				if (i != toCatch->count() - 1)
					*state->l << jmp(end);
			}

			*state->l << end;
		}

		void TryBlock::blockCode(CodeGen *state, CodeResult *to) {
			// Generate code for the entire block. Stop whenever we find a block that does not return.
			for (Nat i = 0; i < exprs->count() - 1; i++) {
				Expr *e = exprs->at(i);
				*state->l << code::location(e->pos);

				CodeResult *s = new (this) CodeResult();
				e->code(state, s);

				// Stop if this statement never returns.
				if (e->result().nothing())
					return;
			}

			*state->l << code::location(exprs->last()->pos);
			Expr *last = expectCastTo(exprs->last(), result().type(), scope);
			last->code(state, to);

			// Generate a last location.
			*state->l << code::location(pos.lastCh());
		}

		ExprResult TryBlock::result() {
			// We're using a fairly simple version of 'common' here.
			ExprResult r = ExprBlock::result();

			for (Nat i = 0; i < toCatch->count(); i++)
				r = common(r, toCatch->at(i)->result());

			return r;
		}

		/**
		 * Catch
		 */

		CatchBlock::CatchBlock(SrcPos pos, Block *parent, SrcName *t, MAYBE(syntax::SStr *) name) : Block(pos, parent) {
			type = as<Type>(parent->scope.find(t));
			if (!type)
				throw new (this) SyntaxError(t->pos, TO_S(this, S("Unable to find the type ") << t << S(".")));

			if (!type->isA(StormInfo<Exception>::type(engine())))
				throw new (this) SyntaxError(t->pos, S("Can only catch classes that inherit from Exception!"));

			if (name) {
				var = new (this) LocalVar(name->v, Value(type), name->pos, false);
				add(var);
			}
		}

		void CatchBlock::expr(Expr *e) {
			run = e;
		}

		void CatchBlock::blockCode(CodeGen *state, CodeResult *to) {
			using namespace code;

			if (var) {
				*state->l << mov(var->var.v, exceptionVar);
			}

			if (run) {
				run->code(state, to);
			}
		}

		ExprResult CatchBlock::result() {
			if (run)
				return run->result();
			else
				return ExprResult();
		}

	}
}
