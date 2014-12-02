#include "stdafx.h"
#include "BSVar.h"
#include "BSBlock.h"

namespace storm {
	namespace bs {

		bs::Var::Var(Auto<Block> block, Auto<TypeName> type, Auto<SStr> name) {
			Value t = type->value(block->scope);
			variable = CREATE(LocalVar, this, name->v->v, t, name->pos);
			block->add(variable);
		}

		bs::Var::Var(Auto<Block> block, Auto<SStr> name, Auto<Expr> init) {
			Value t = init->result();
			this->init = init;
			variable = CREATE(LocalVar, this, name->v->v, t, name->pos);
			block->add(variable);
		}

		void bs::Var::initTo(Auto<Expr> e) {
			init = e;
		}

		Value bs::Var::result() {
			return variable->result;
		}

		void bs::Var::code(const GenState &s, code::Variable to) {
			using namespace code;

			assert(variable->var != Variable::invalid);

			if (init) {
				// Evaluate.
				init->code(s, variable->var);
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
