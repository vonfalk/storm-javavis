#include "stdafx.h"
#include "Block.h"
#include "Exception.h"
#include "NamePart.h"
#include "Core/StrBuf.h"

namespace storm {
	namespace bs {

		Block::Block(SrcPos pos, Scope scope) : Expr(pos) {
			lookup = new (this) BlockLookup(this, scope.top);
			this->scope = Scope(scope, lookup);
			variables = new (this) VarMap();
		}

		Block::Block(SrcPos pos, Block *parent) : Expr(pos) {
			lookup = new (this) BlockLookup(this, parent->scope.top);
			scope = Scope(parent->scope, lookup);
			variables = new (this) VarMap();
		}

		void Block::code(CodeGen *state, CodeResult *to) {
			using namespace code;

			code::Block block = state->l->createBlock(state->l->last(state->block));
			CodeGen *child = state->child(block);

			for (VarMap::Iter i = variables->begin(), end = variables->end(); i != end; ++i) {
				i.v()->create(child);
			}

			blockCode(state, to, block);

			// May be delayed...
			if (to->needed())
				to->location(state).created(state);
		}

		void Block::blockCode(CodeGen *state, CodeResult *to, const code::Block &block) {
			*state->l << begin(block);
			CodeGen *subState = state->child(block);
			blockCode(subState, to);
			*state->l << end(block);
		}

		void Block::blockCode(CodeGen *state, CodeResult *to) {
			assert(false, "Implement me in a subclass!");
		}

		void Block::add(LocalVar *var) {
			if (variables->has(var->name)) {
				// Old position: variables->get(var->name)->pos
				throw TypeError(var->pos, L"The variable " + ::toS(var->name) + L" is already defined in this block:\n@" +
								::toS(variables->get(var->name)->pos) + L": Syntax error: Previously defined here.");
			}
			variables->put(var->name, var);
		}

		void Block::liftVars(Block *from) {
			assert(isParentTo(from), L"'liftVars' can only be used to move variables from child to parent, one step.");

			for (VarMap::Iter i = from->variables->begin(); i != from->variables->end(); ++i) {
				add(i.v());
			}

			from->variables->clear();
		}

		bool Block::isParentTo(Block *x) {
			return x->lookup->parent() == lookup;
		}

		LocalVar *Block::variable(SimplePart *part) {
			// Don't try to search for parameterized variables, that makes no sense at the moment!
			if (part->params->any())
				return null;

			Str *name = part->name;
			VarMap::Iter i = variables->find(name);
			if (i != variables->end())
				return i.v();

			BlockLookup *parent = as<BlockLookup>(lookup->parent());
			if (parent)
				return parent->block->variable(part);

			return null;
		}


		ExprBlock::ExprBlock(SrcPos pos, Scope scope) :
			Block(pos, scope), exprs(new (this) Array<Expr *>()), firstNoReturn(invalid) {}

		ExprBlock::ExprBlock(SrcPos pos, Block *parent) :
			Block(pos, parent), exprs(new (this) Array<Expr *>()), firstNoReturn(invalid) {}

		void ExprBlock::add(Expr *expr) {
			if (firstNoReturn == invalid)
				if (expr->result().nothing())
					firstNoReturn = exprs->count();

			exprs->push(expr);
		}

		void ExprBlock::insert(Nat pos, Expr *expr) {
			if (firstNoReturn != invalid) {
				if (firstNoReturn >= pos)
					firstNoReturn++;
			}

			if (firstNoReturn == invalid || firstNoReturn >= pos)
				if (expr->result().nothing())
					firstNoReturn = pos;

			exprs->insert(pos, expr);
		}

		ExprResult ExprBlock::result() {
			if (firstNoReturn != invalid) {
				return noReturn();
			} else if (exprs->any()) {
				return exprs->last()->result();
			} else {
				return ExprResult();
			}
		}

		void ExprBlock::code(CodeGen *state, CodeResult *to) {
			if (exprs->any())
				Block::code(state, to);
		}

		void ExprBlock::blockCode(CodeGen *state, CodeResult *to) {
			if (firstNoReturn == invalid) {
				// Generate code for the entire block. Skip the last expression, as that is supposed to
				// return something.
				for (nat i = 0; i < exprs->count() - 1; i++) {
					CodeResult *s = new (this) CodeResult();
					exprs->at(i)->code(state, s);
				}

				// Pass the return value on to the last expression.
				exprs->last()->code(state, to);

			} else {
				// Generate code until the first dead block.
				for (nat i = 0; i <= firstNoReturn; i++) {
					CodeResult *s = new (this) CodeResult();
					exprs->at(i)->code(state, s);
				}
			}
		}

		Int ExprBlock::castPenalty(Value to) {
			if (exprs->empty())
				return -1;
			return exprs->last()->castPenalty(to);
		}

		void ExprBlock::toS(StrBuf *to) const {
			*to << S("{\n");
			{
				Indent i(to);
				for (nat i = 0; i < exprs->count(); i++) {
					*to << exprs->at(i) << S(";\n");
					if (i == firstNoReturn && i != exprs->count() - 1)
						*to << S("// unreachable code:\n");
				}
			}
			*to << S("}");
		}

		Expr *ExprBlock::operator [](Nat at) const {
			return exprs->at(at);
		}

		Nat ExprBlock::count() const {
			return exprs->count();
		}

		/**
		 * Block lookup
		 */

		BlockLookup::BlockLookup(Block *o, NameLookup *prev) : NameLookup(prev), block(o) {}

		Named *BlockLookup::find(SimplePart *part, Scope source) {
			if (part->params->empty())
				return block->variable(part);

			return null;
		}

	}
}
