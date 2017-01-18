#include "stdafx.h"
#include "Var.h"
#include "Block.h"
#include "Cast.h"
#include "Named.h"
#include "Type.h"
#include "Exception.h"

namespace storm {
	namespace bs {

		Var::Var(Block *block, SrcName *type, syntax::SStr *name, Actuals *params) : Expr(name->pos) {
			init(block, block->scope.value(type), name);
			initTo(params);
		}

		Var::Var(Block *block, Value type, syntax::SStr *name, Actuals *params) : Expr(name->pos) {
			init(block, type.asRef(false), name);
			initTo(params);
		}

		Var::Var(Block *block, SrcName *type, syntax::SStr *name, Expr *init) : Expr(name->pos) {
			this->init(block, block->scope.value(type), name);
			initTo(init);
		}

		Var::Var(Block *block, Value type, syntax::SStr *name, Expr *init) : Expr(name->pos) {
			this->init(block, type.asRef(false), name);
			initTo(init);
		}

		Var::Var(Block *block, syntax::SStr *name, Expr *init) : Expr(name->pos) {
			this->init(block, init->result().type().asRef(false), name);
			initTo(init);
		}

		LocalVar *Var::var() {
			return variable;
		}

		void Var::init(Block *block, const Value &type, syntax::SStr *name) {
			variable = new (this) LocalVar(name->v, type, name->pos, false);
			block->add(variable);
		}

		void Var::initTo(Expr *e) {
			if (Expr *z = castTo(e, variable->result))
				initExpr = z;
			else
				// Use a ctor...
				initTo(new (this) Actuals(e));
		}

		void Var::initTo(Actuals *actuals) {
			if (variable->result.isBuiltIn()) {
				// Assignment is the same as initialization here...
				nat size = actuals->expressions->count();
				if (size == 1) {
					initExpr = actuals->expressions->at(0);
					return;
				} else if (size == 0) {
					// No constructor, initialized to zero!
					return;
				}
			}

			Type *t = variable->result.type;
			BSNamePart *name = new (this) BSNamePart(Type::CTOR, pos, actuals);
			name->insert(thisPtr(t));
			Function *ctor = as<Function>(t->find(name));
			if (!ctor)
				throw SyntaxError(variable->pos, L"No constructor " + ::toS(variable->result)
								+ L"(" + ::toS(name) + L") found. Can not initialize "
								+ ::toS(variable->name) + L".");

			initCtor = new (this) CtorCall(pos, ctor, actuals);
		}

		ExprResult Var::result() {
			return variable->result.asRef();
		}

		void Var::code(CodeGen *s, CodeResult *to) {
			using namespace code;

			const Value &t = variable->result;

			if (t.isValue()) {
				Expr *ctor = null;

				if (initCtor)
					ctor = initCtor;
				else if (initExpr)
					ctor = copyCtor(pos, t.type, initExpr);
				else
					ctor = defaultCtor(pos, t.type);

				if (ctor) {
					CodeResult *gr = new (this) CodeResult(variable->result, variable->var);
					ctor->code(s, gr);
				}
			} else if (initExpr) {
				CodeResult *gr = new (this) CodeResult(variable->result, variable->var);
				initExpr->code(s, gr);
			} else if (initCtor) {
				CodeResult *gr = new (this) CodeResult(variable->result, variable->var);
				initCtor->code(s, gr);
			}

			variable->var.created(s);

			if (to->needed()) {
				// Part of another expression.
				if (to->type().ref) {
					VarInfo v = to->location(s);
					*s->to << lea(v.v, variable->var.v);
					v.created(s);
				} else if (!to->suggest(s, variable->var.v)) {
					VarInfo v = to->location(s);
					*s->to << mov(v.v, variable->var.v);
					v.created(s);
				}
			}
		}

		void Var::toS(StrBuf *to) const {
			if (variable->constant)
				*to << L"const ";
			*to << variable->result << L" " << variable->name;
			if (initExpr)
				*to << L" = " << initExpr;
			else if (initCtor)
				*to << L"(" << initCtor << L")";
		}


		/**
		 * LocalVar
		 */

		LocalVar::LocalVar(Str *name, Value val, SrcPos pos, Bool param)
			: Named(name), result(val), pos(pos), var(), param(param), constant(false) {}

		void LocalVar::create(CodeGen *state) {
			if (param)
				return;

			if (var.v != code::Var()) {
				assert(state->to->accessible(var.v, state->block));
			} else {
				var = state->createVar(result);
			}
		}

		void LocalVar::createParam(CodeGen *state) {
			using namespace code;

			if (!param)
				return;
			assert(var.v == code::Var(), L"Already created!");

			var = VarInfo(state->createParam(result));
		}


	}
}
