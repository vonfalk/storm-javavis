#include "stdafx.h"
#include "BSVar.h"
#include "BSBlock.h"
#include "BSNamed.h"
#include "BSAutocast.h"
#include "Exception.h"
#include "Lib/Debug.h"

namespace storm {
	namespace bs {

		bs::Var::Var(Par<Block> block, Par<SrcName> type, Par<SStr> name, Par<Actual> params) : Expr(name->pos) {
			init(block, block->scope.value(type), name);
			initTo(params);
		}

		bs::Var::Var(Par<Block> block, Value type, Par<SStr> name, Par<Actual> params) : Expr(name->pos) {
			init(block, type.asRef(false), name);
			initTo(params);
		}

		bs::Var::Var(Par<Block> block, Par<SrcName> type, Par<SStr> name, Par<Expr> init) : Expr(name->pos) {
			this->init(block, block->scope.value(type), name);
			initTo(init);
		}

		bs::Var::Var(Par<Block> block, Value type, Par<SStr> name, Par<Expr> init) : Expr(name->pos) {
			this->init(block, type.asRef(false), name);
			initTo(init);
		}

		bs::Var::Var(Par<Block> block, Par<SStr> name, Par<Expr> init) : Expr(name->pos) {
			this->init(block, init->result().type().asRef(false), name);
			initTo(init);
		}

		bs::LocalVar *bs::Var::var() {
			return variable.ret();
		}

		void bs::Var::init(Par<Block> block, const Value &type, Par<SStr> name) {
			variable = CREATE(LocalVar, this, name->v->v, type, name->pos);
			block->add(variable);
		}

		void bs::Var::initTo(Par<Expr> e) {
			if (Expr *z = castTo(e, variable->result))
				initExpr = z;
			else
				// Use a ctor...
				initTo(steal(CREATE(Actual, engine(), e)));
		}

		void bs::Var::initTo(Par<Actual> actuals) {
			if (variable->result.isBuiltIn()) {
				// Assignment is the same as initialization here...
				nat size = actuals->expressions.size();
				if (size == 1) {
					initExpr = actuals->expressions[0];
					return;
				} else if (size == 0) {
					// No constructor, initialized to zero!
					return;
				}
			}

			Type *t = variable->result.type;
			Auto<BSNamePart> name = CREATE(BSNamePart, this, Type::CTOR, pos, actuals);
			name->insert(Value::thisPtr(t));
			Auto<Function> ctor = steal(t->find(name)).as<Function>();
			if (!ctor)
				throw SyntaxError(variable->pos, L"No constructor " + ::toS(variable->result)
								+ L"(" + ::toS(name) + L") found. Can not initialize "
								+ variable->name + L".");

			initCtor = CREATE(CtorCall, this, pos, ctor, actuals);
		}

		ExprResult bs::Var::result() {
			return variable->result.asRef();
		}

		void bs::Var::code(Par<CodeGen> s, Par<CodeResult> to) {
			using namespace code;

			const Value &t = variable->result;

			if (t.isValue()) {
				Auto<Expr> ctor;

				if (initCtor)
					ctor = initCtor;
				else if (initExpr)
					ctor = copyCtor(pos, t.type, initExpr);
				else
					ctor = defaultCtor(pos, t.type);

				if (ctor) {
					Auto<CodeResult> gr = CREATE(CodeResult, this, variable->result, variable->var);
					ctor->code(s, gr);
				}
			} else if (initExpr) {
				Auto<CodeResult> gr = CREATE(CodeResult, this, variable->result, variable->var);
				initExpr->code(s, gr);
			} else if (initCtor) {
				Auto<CodeResult> gr = CREATE(CodeResult, this, variable->result, variable->var);
				initCtor->code(s, gr);
			}

			variable->var.created(s);

			if (to->needed()) {
				// Part of another expression.
				if (to->type().ref) {
					VarInfo v = to->location(s);
					s->to << lea(v.var(), variable->var.var());
					v.created(s);
				} else if (!to->suggest(s, variable->var.var())) {
					VarInfo v = to->location(s);
					s->to << mov(v.var(), variable->var.var());
					if (variable->result.refcounted())
						s->to << code::addRef(v.var());
					v.created(s);
				}
			}
		}

		void bs::Var::output(wostream &to) const {
			if (variable->constant)
				to << L"const ";
			to << variable->result << L" " << variable->name;
			if (initExpr)
				to << L" = " << initExpr;
			else if (initCtor)
				to << L"(" << initCtor << L")";
		}


		/**
		 * LocalVar
		 */

		bs::LocalVar::LocalVar(const String &name, const Value &t, const SrcPos &pos, bool param)
			: Named(name), result(t), pos(pos), var(code::Variable::invalid), param(param), constant(false) {}

		bs::LocalVar::LocalVar(Par<Str> name, Value val, SrcPos pos, Bool param)
			: Named(name->v), result(val), pos(pos), var(code::Variable::invalid), param(param), constant(false) {}

		void LocalVar::create(Par<CodeGen> state) {
			if (param)
				return;

			if (var.var() != code::Variable::invalid) {
				assert(state->frame.accessible(state->block.v, var.var()));
			} else {
				var = storm::variable(state->frame, state->block.v, result);
			}
		}

		void LocalVar::createParam(Par<CodeGen> state) {
			using namespace code;

			if (!param)
				return;
			assert(var.var() == Variable::invalid, L"Already created!");

			Variable z = state->addParam(result, !constant).v;
			var = VarInfo(z);
		}

	}
}
