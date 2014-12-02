#include "stdafx.h"
#include "BSVar.h"
#include "BSBlock.h"

namespace storm {
	namespace bs {

		bs::Var::Var(Auto<Block> block, Auto<TypeName> type, Auto<SStr> name) {
			init(block, type->value(block->scope), name);
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

		void bs::Var::code(const GenState &s, code::Variable to) {
			using namespace code;

			assert(variable->var != Variable::invalid);

			if (initExpr) {
				// Evaluate.
				initExpr->code(s, variable->var);
			}


			if (to != Variable::invalid) {
				// Part of another expression.
				s.to << mov(to, variable->var);
				if (variable->result.refcounted())
					s.to << code::addRef(to);
			}
		}


		/**
		 * LocalVar
		 */

		bs::LocalVar::LocalVar(const String &name, const Value &t, const SrcPos &pos)
			: Named(name), result(t), pos(pos), var(code::Variable::invalid) {}

	}
}
