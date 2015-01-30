#include "stdafx.h"
#include "BSVar.h"
#include "BSBlock.h"
#include "BSNamed.h"
#include "Exception.h"

namespace storm {
	namespace bs {

		bs::Var::Var(Par<Block> block, Par<TypeName> type, Par<SStr> name) {
			init(block, type->value(block->scope), name);
		}

		bs::Var::Var(Par<Block> block, Par<TypeName> type, Par<SStr> name, Par<Actual> params) {
			init(block, type->value(block->scope), name);
			initTo(params);
		}

		bs::Var::Var(Par<Block> block, Par<TypeName> type, Par<SStr> name, Par<Expr> init) {
			this->init(block, type->value(block->scope), name);
			initTo(init);
		}

		bs::Var::Var(Par<Block> block, Par<SStr> name, Par<Expr> init) {
			this->init(block, init->result(), name);
			initTo(init);
		}

		void bs::Var::init(Par<Block> block, const Value &type, Par<SStr> name) {
			variable = CREATE(LocalVar, this, name->v->v, type, name->pos);
			block->add(variable);
		}

		void bs::Var::initTo(Par<Expr> e) {
			variable->result.mustStore(e->result(), variable->pos);
			initExpr = e;
		}

		void bs::Var::initTo(Par<Actual> actuals) {
			if (variable->result.isBuiltIn()) {
				// Assignment is the same as initialization here...
				if (actuals->expressions.size() == 1) {
					initTo(Par<Expr>(actuals->expressions[0]));
					return;
				}
			}

			Type *t = variable->result.type;
			Overload *ctors = as<Overload>(t->find(Type::CTOR));
			if (!ctors)
				throw SyntaxError(pos, ::toS(variable->result) + L" has no constructors.");

			vector<Value> params = actuals->values();
			params.insert(params.begin(), Value::thisPtr(t));
			Function *ctor = as<Function>(ctors->find(params));
			if (!ctor)
				throw SyntaxError(pos, L"No constructor " + ::toS(variable->result) + L"("
								+ join(params, L", ") + L") found. Can not initialize.");

			initCtor = CREATE(CtorCall, this, capture(ctor), actuals);
		}

		Value bs::Var::result() {
			return variable->result.asRef();
		}

		void bs::Var::code(const GenState &s, GenResult &to) {
			using namespace code;

			assert(variable->var != Variable::invalid);

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
					GenResult gr(variable->result, variable->var);
					ctor->code(s, gr);
				}
			} else if (initExpr) {
				GenResult gr(variable->result, variable->var);
				initExpr->code(s, gr);
			} else if (initCtor) {
				GenResult gr(variable->result, variable->var);
				initCtor->code(s, gr);
			}

			if (to.needed()) {
				// Part of another expression.
				if (to.type.ref) {
					Variable v = to.location(s);
					s.to << lea(v, variable->var);
				} else if (!to.suggest(s, variable->var)) {
					Variable v = to.location(s);

					s.to << mov(v, variable->var);
					if (variable->result.refcounted())
						s.to << code::addRef(v);
				}
			}
		}


		/**
		 * LocalVar
		 */

		bs::LocalVar::LocalVar(const String &name, const Value &t, const SrcPos &pos, bool param)
			: Named(name), result(t), pos(pos), var(code::Variable::invalid), param(param) {}

	}
}
