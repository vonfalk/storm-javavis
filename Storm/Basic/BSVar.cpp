#include "stdafx.h"
#include "BSVar.h"
#include "BSBlock.h"
#include "BSNamed.h"
#include "BSAutocast.h"
#include "Exception.h"
#include "Lib/Debug.h"

namespace storm {
	namespace bs {

		bs::Var::Var(Par<Block> block, Par<TypeName> type, Par<SStr> name, Par<Actual> params) {
			init(block, type->resolve(block->scope), name);
			initTo(params);
		}

		bs::Var::Var(Par<Block> block, Par<TypeName> type, Par<SStr> name, Par<Expr> init) {
			this->init(block, type->resolve(block->scope), name);
			initTo(init);
		}

		bs::Var::Var(Par<Block> block, Par<SStr> name, Par<Expr> init) {
			this->init(block, init->result().asRef(false), name);
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
			vector<Value> params = actuals->values();
			params.insert(params.begin(), Value::thisPtr(t));
			Function *ctor = as<Function>(t->findCpp(Type::CTOR, params));
			if (!ctor)
				throw SyntaxError(variable->pos, L"No constructor " + ::toS(variable->result)
								+ L"(" + join(params, L", ") + L") found. Can not initialize "
								+ variable->name + L".");

			initCtor = CREATE(CtorCall, this, capture(ctor), actuals);
		}

		Value bs::Var::result() {
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

		bs::LocalVar::LocalVar(Par<Str> name, const Value &val, const SrcPos &pos, Bool param)
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

			Variable z;
			if (result.isValue()) {
				z = state->frame.createParameter(result.size(), false, result.destructor(), freeOnBoth | freePtr);
			} else if (constant) {
				// Borrowed ptr.
				z = state->frame.createParameter(result.size(), false);
			} else {
				z = state->frame.createParameter(result.size(), false, result.destructor());
			}

			var = VarInfo(z);

			if (result.refcounted() && !constant)
				state->to << code::addRef(var.var());
		}

	}
}
