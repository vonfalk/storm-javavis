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

			code::Block block = state->to->createBlock(state->to->last(state->block));
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
			*state->to << begin(block);
			CodeGen *subState = state->child(block);
			blockCode(subState, to);
			*state->to << end(block);
		}

		void Block::blockCode(CodeGen *state, CodeResult *to) {
			assert(false, "Implement me in a subclass!");
		}

		void Block::add(LocalVar *var) {
			if (variables->has(var->name))
				throw TypeError(variables->get(var->name)->pos, L"The variable " + ::toS(var->name) + L" is already defined.");
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
				if (expr->result().empty())
					firstNoReturn = exprs->count();

			exprs->push(expr);
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
			*to << L"{\n";
			{
				Indent i(to);
				for (nat i = 0; i < exprs->count(); i++) {
					*to << exprs->at(i) << L";\n";
					if (i == firstNoReturn && i != exprs->count() - 1)
						*to << L"// unreachable code:\n";
				}
			}
			*to << L"}";
		}

		/**
		 * Block lookup
		 */

		BlockLookup::BlockLookup(Block *o, NameLookup *prev) : block(o) {
			parentLookup = prev;
		}

		Named *BlockLookup::find(SimplePart *part) {
			if (part->params->empty())
				return block->variable(part);

			return null;
		}

	}
}