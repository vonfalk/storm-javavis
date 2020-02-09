#include "stdafx.h"
#include "Breakable.h"
#include "Compiler/Exception.h"

namespace storm {
	namespace bs {

		Breakable::Breakable(SrcPos pos, Scope scope) : Block(pos, scope) {}

		Breakable::Breakable(SrcPos pos, Block *parent) : Block(pos, parent) {}

		Breakable::To::To(code::Label lbl, code::Block block) : label(lbl), block(block) {}

		static MAYBE(Breakable *) findBreakable(Block *block) {
			BlockLookup *lookup = block->lookup;

			while (lookup) {
				Block *block = lookup->block;
				if (Breakable *b = as<Breakable>(block))
					return b;

				lookup = as<BlockLookup>(lookup->parent());
			}

			return null;
		}

		Break::Break(SrcPos pos, Block *parent) : Expr(pos) {
			if (!(breakFrom = findBreakable(parent)))
				throw new (this) SyntaxError(pos, S("Nothing to break from here. Use break inside loops."));

			breakFrom->willBreak();
		}

		void Break::code(CodeGen *state, CodeResult *r) {
			Breakable::To to = breakFrom->breakTo();
			*state->l << jmpBlock(to.label, to.block);
		}

		Bool Break::isolate() {
			return false;
		}


		Continue::Continue(SrcPos pos, Block *parent) : Expr(pos) {
			if (!(continueIn = findBreakable(parent)))
				throw new (this) SyntaxError(pos, S("Nothing to continue from here. Use continue inside loops."));

			continueIn->willContinue();
		}

		void Continue::code(CodeGen *state, CodeResult *r) {
			Breakable::To to = continueIn->continueTo();
			*state->l << jmpBlock(to.label, to.block);
		}

		Bool Continue::isolate() {
			return false;
		}

	}
}
