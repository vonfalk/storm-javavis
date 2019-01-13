#include "stdafx.h"
#include "Operators.h"
#include "Named.h"
#include "Cast.h"
#include "Resolve.h"
#include "Compiler/Exception.h"
#include "Utils/Bitmask.h"

namespace storm {
	namespace bs {

		AssignOpInfo::AssignOpInfo(syntax::SStr *op, Int prio, Bool leftAssoc) : OpInfo(op, prio, leftAssoc) {}

		Expr *AssignOpInfo::meaning(Block *block, Expr *lhs, Expr *rhs) {
			// We need to do this first. Otherwise the UnresolvedName will barf when we ask it for its return type.
			if (as<UnresolvedName>(lhs)) {
				// Try to find a setter function!
				if (Expr *setter = findSetter(block, lhs, rhs))
					return setter;
			}

			// Notify LHS that we are going to assign to it, so that it can tell the user if that is a bad idea.
			if (MemberVarAccess *access = as<MemberVarAccess>(lhs)) {
				access->assignResult();
			}

			Value l = lhs->result().type();
			// Note: We will check compatibility between types at a later stage.

			if (!l.ref) {
				// Try to use a setter function if we can find one.
				if (Expr *setter = findSetter(block, lhs, rhs))
					return setter;

				throw SyntaxError(pos, L"Unable to assign to the value " + ::toS(l) + L". "
								L"It is only possible to assign to references (such as variables), "
								L"and if an assignment function is available.");
			}

			if (l.isObject() && l.ref && castable(rhs, l.asRef(false), block->scope)) {
				return new (block) ClassAssign(lhs, rhs, block->scope);
			} else {
				// Make sure we do not allow automatic conversion of the 'this' parameter during
				// assignment. That would produce weird results.
				Expr *fn = find(block, name, lhs, rhs, true);
				if (!fn)
					throw SyntaxError(pos, L"Can not find an implementation of the operator " +
									::toS(name) + L" for " + ::toS(lhs->result().type()) + L", " +
									::toS(rhs->result().type()) + L".");
				return fn;
			}
		}

		Expr *AssignOpInfo::findSetter(Block *block, Expr *lhs, Expr *rhs) {
			UnresolvedName *name = as<UnresolvedName>(lhs);
			if (!name)
				if (FnCall *call = as<FnCall>(lhs))
					name = call->name();

			// Not something we can use to find a setter.
			if (!name)
				return null;

			Actuals *params = new (this) Actuals(*name->params);
			params->add(rhs);

			// Try again!
			FnCall *call = as<FnCall>(name->retry(params));
			if (!call)
				return null;

			// Only accept functions explicitly marked as 'assign'.
			if (call->function()->fnFlags() & fnAssign)
				return call;

			return null;
		}

		OpInfo *assignOperator(syntax::SStr *op, Int p) {
			return new (op) AssignOpInfo(op, p, false);
		}


		/**
		 * Is operator.
		 */

		IsOperator::IsOperator(syntax::SStr *op, Int prio, Bool negate) : OpInfo(op, prio, true), negate(negate) {}

		Expr *IsOperator::meaning(Block *block, Expr *lhs, Expr *rhs) {
			return new (this) ClassCompare(pos, lhs, rhs, negate);
		}


		/**
		 * Combined operator.
		 */

		static Str *combinedName(Str *name) {
			return *name + new (name) Str(L"=");
		}

		CombinedOperator::CombinedOperator(OpInfo *info, Int prio) :
			OpInfo(new (engine()) syntax::SStr(combinedName(info->name)), prio, true), op(info) {}

		Expr *CombinedOperator::meaning(Block *block, Expr *lhs, Expr *rhs) {
			// See if the combined operator exists first.
			if (Expr *r = find(block, name, lhs, rhs))
				return r;

			try {
				// Create: 'lhs = lhs <op> rhs'
				syntax::SStr *eqOp = new (this) syntax::SStr(S("="), pos);
				OpInfo *assign = new (this) AssignOpInfo(eqOp, 100, true);

				Expr *middle = op->meaning(block, lhs, rhs);
				return assign->meaning(block, lhs, middle);
			} catch (...) {
				TODO(L"Better error message when using combined operators.");
				throw;
			}
		}


		/**
		 * Fallback operator.
		 */

		enum FallbackFlags {
			// Swap the order of the arguments?
			fSwap = 0x01,

			// Negate the result?
			fNegate = 0x02,

			// Connect with the next entry using '&'?
			fConnectAnd = 0x04,

			// Connect with the next entry using '|'?
			fConnectOr  = 0x08,
		};
		BITMASK_OPERATORS(FallbackFlags);

		struct OpFallback {
			// Name of the operator to use. 'null' if no more elements.
			const wchar *op;

			// Flags.
			FallbackFlags flags;
		};

		FallbackOperator::FallbackOperator(syntax::SStr *op, Int prio, Bool leftAssoc, const OpFallback *fallback) :
			OpInfo(op, prio, leftAssoc), fallback(fallback) {}

		Expr *FallbackOperator::meaning(Block *block, Expr *lhs, Expr *rhs) {
			// Try the original meaning first.
			if (Expr *r = find(block, name, lhs, rhs))
				return r;

			// Interpret the 'fallback' data and try those in order.
			for (Nat i = 0; fallback[i].op; i++)
				if (Expr *r = tryFallback(i, block, lhs, rhs))
					return r;

			throw SyntaxError(pos, L"Can not find an implementation of the operator " +
							::toS(name) + L" for " + ::toS(lhs->result().type()) + L", " +
							::toS(rhs->result().type()) + L".");
		}

		Expr *FallbackOperator::tryFallback(Nat &id, Block *block, Expr *lhs, Expr *rhs) {
			const OpFallback &f = fallback[id];
			if (f.flags & fConnectAnd) {
				Expr *a = tryFallback(fallback[id++], block, lhs, rhs);
				Expr *b = tryFallback(fallback[id], block, lhs, rhs);

				if (a && b)
					return namedExpr(block, new (this) syntax::SStr(S("&"), pos), a, new (this) Actuals(b));

				return null;
			} else if (f.flags & fConnectOr) {
				Expr *a = tryFallback(fallback[id++], block, lhs, rhs);
				Expr *b = tryFallback(fallback[id], block, lhs, rhs);

				if (a && b)
					return namedExpr(block, new (this) syntax::SStr(S("|"), pos), a, new (this) Actuals(b));

				return null;
			} else {
				return tryFallback(f, block, lhs, rhs);
			}
		}

		Expr *FallbackOperator::tryFallback(const OpFallback &f, Block *block, Expr *lhs, Expr *rhs) {
			// Swap lhs and rhs?
			if (f.flags & fSwap)
				std::swap(lhs, rhs);

			Expr *r = find(block, new (this) Str(f.op), lhs, rhs);
			if (!r)
				return null;

			// Add boolean ! after the comparison?
			if (f.flags & fNegate)
				r = namedExpr(block, new (this) syntax::SStr(S("!"), pos), r);

			return r;
		}


		/**
		 * Definitions of how the fallback is to be performed. The original meaning of the operator
		 * is always attempted first. Otherwise, we will prefer to use '<' over any variants of '>'
		 * if possible.
		 */

		static const OpFallback ltFallback[] = {
			{ S(">"), fSwap }, // b > a
			{ null },
		};

		static const OpFallback gtFallback[] = {
			{ S("<"), fSwap }, // b < a
			{ null },
		};

		static const OpFallback lteFallback[] = {
			{ S("<"), fSwap | fNegate }, // !(b < a)
			{ S(">"), fNegate }, // !(a > b)
			{ null },
		};

		static const OpFallback gteFallback[] = {
			{ S("<"), fNegate }, // !(a < b)
			{ S(">"), fSwap | fNegate }, // !(b > a)
			{ null },
		};

		static const OpFallback eqFallback[] = {
			// !(a < b) && !(b < a)
			{ S("<"), fNegate | fConnectAnd },
			{ S("<"), fSwap | fNegate },
			// !(a > b) && !(b > a)
			{ S("<"), fNegate | fConnectAnd },
			{ S("<"), fSwap | fNegate },
			{ null },
		};

		static const OpFallback neqFallback[] = {
			// !(a == b)
			{ S("=="), fNegate },
			// (a < b) || (b < a)
			{ S("<"), fConnectOr },
			{ S("<"), fSwap },
			// !(a > b) && !(b > a)
			{ S("<"), fConnectOr },
			{ S("<"), fSwap },
			{ null },
		};


		OpInfo *compareLt(syntax::SStr *op, Int priority) {
			return new (op) FallbackOperator(op, priority, true, ltFallback);
		}

		OpInfo *compareLte(syntax::SStr *op, Int priority) {
			return new (op) FallbackOperator(op, priority, true, lteFallback);
		}

		OpInfo *compareGt(syntax::SStr *op, Int priority) {
			return new (op) FallbackOperator(op, priority, true, gtFallback);
		}

		OpInfo *compareGte(syntax::SStr *op, Int priority) {
			return new (op) FallbackOperator(op, priority, true, gteFallback);
		}

		OpInfo *compareEq(syntax::SStr *op, Int priority) {
			return new (op) FallbackOperator(op, priority, true, eqFallback);
		}

		OpInfo *compareNeq(syntax::SStr *op, Int priority) {
			return new (op) FallbackOperator(op, priority, true, neqFallback);
		}

	}
}
