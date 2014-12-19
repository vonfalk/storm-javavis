#include "stdafx.h"
#include "BSVar.h"
#include "BSBlock.h"

namespace storm {
	namespace bs {

		bs::Var::Var(Auto<Block> block, Auto<TypeName> type, Auto<SStr> name) {
			init(block, type->value(block->scope), name);
		}

		bs::Var::Var(Auto<Block> block, Auto<TypeName> type, Auto<SStr> name, Auto<Expr> init) {
			this->init(block, type->value(block->scope), name);
			initTo(init);
		}

		bs::Var::Var(Auto<Block> block, Auto<SStr> name, Auto<Expr> init) {
			this->init(block, init->result(), name);
			initTo(init);
		}

		void bs::Var::init(Auto<Block> block, const Value &type, Auto<SStr> name) {
			variable = CREATE(LocalVar, this, name->v->v, type, name->pos);
			block->add(variable);
		}

		void bs::Var::initTo(Auto<Expr> e) {
			variable->result.mustStore(e->result(), variable->pos);
			initExpr = e;
		}

		Value bs::Var::result() {
			return variable->result;
		}

		void bs::Var::code(const GenState &s, GenResult &to) {
			using namespace code;

			assert(variable->var != Variable::invalid);

			if (initExpr) {
				// Evaluate.
				GenResult gr(variable->var);
				initExpr->code(s, gr);
			}


			if (to.needed) {
				// Part of another expression.
				if (!to.suggest(s, variable->var)) {
					Variable v = to.location(s, variable->result);

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
