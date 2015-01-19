#include "stdafx.h"
#include "BSVar.h"
#include "BSBlock.h"

namespace storm {
	namespace bs {

		bs::Var::Var(Par<Block> block, Par<TypeName> type, Par<SStr> name) {
			init(block, type->value(block->scope), name);
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

		Value bs::Var::result() {
			return variable->result.asRef();
		}

		void bs::Var::code(const GenState &s, GenResult &to) {
			using namespace code;

			assert(variable->var != Variable::invalid);

			if (initExpr) {
				// Evaluate.
				GenResult gr(variable->result, variable->var);
				initExpr->code(s, gr);
			}


			if (to.needed()) {
				// Part of another expression.
				if (to.type.ref) {
					Variable v = to.location(s);
					s.to << lea(v, variable->var);
				} else if (!to.suggest(s, variable->var)) {
					Variable v = to.location(s);

					if (variable->result.refcounted())
						s.to << code::addRef(v);
					s.to << mov(v, variable->var);
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
