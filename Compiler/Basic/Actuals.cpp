#include "stdafx.h"
#include "Actuals.h"
#include "Cast.h"
#include "Compiler/CodeGen.h"

namespace storm {
	namespace bs {

		Actuals::Actuals() {
			expressions = new (this) Array<Expr *>();
		}

		Actuals::Actuals(Expr *expr) {
			expressions = new (this) Array<Expr *>();
			add(expr);
		}

		Array<Value> *Actuals::values() {
			Array<Value> *v = new (this) Array<Value>();
			v->reserve(expressions->count());
			for (nat i = 0; i < expressions->count(); i++)
				v->push(expressions->at(i)->result().type());

			return v;
		}

		void Actuals::add(Expr *e) {
			expressions->push(e);
		}

		void Actuals::addFirst(Expr *e) {
			expressions->insert(0, e);
		}

		/**
		 * Helper to compute an actual parameter. Takes care of ref/non-ref conversions.
		 * Returns the value into which the resulting parameter were placed.
		 */
		code::Operand Actuals::code(nat id, CodeGen *s, Value param) {
			using namespace code;

			Expr *expr = castTo(expressions->at(id), param);
			if (param.ref) {
				// If we failed, try to cast to a non-reference type and deal with that later.
				expr = castTo(expressions->at(id), param.asRef(false));
			}
			assert(expr,
				L"Can not use " + ::toS(expressions->at(id)->result()) + L" as an actual value for parameter " + ::toS(param));

			Value exprResult = expr->result().type();
			if (param.ref && !exprResult.ref) {
				// We need to create a temporary variable and make a reference to it.
				exprResult = param.asRef(false); // We need this since type casting can be done late by the Expr itself.
				VarInfo tmpV = s->createVar(exprResult);
				CodeResult *gr = new (this) CodeResult(exprResult, tmpV);
				expr->code(s, gr);

				VarInfo tmpRef = s->createVar(param);
				*s->to << lea(tmpRef.v, ptrRel(tmpV.v, Offset()));
				tmpRef.created(s);
				return tmpRef.v;
			} else {
				// 'expr' will handle the type we are giving it.
				CodeResult *gr = new (this) CodeResult(param, s->block);
				expr->code(s, gr);
				return gr->location(s).v;
			}
		}

		void Actuals::toS(StrBuf *to) const {
			*to << L"(";
			join(to, expressions, L", ");
			*to << L")";
		}


		BSNamePart::BSNamePart(syntax::SStr *name, Actuals *params)
			: SimplePart(name->v, params->values()), pos(name->pos) {

			exprs = new (this) Array<Expr *>(params->expressions);
		}

		BSNamePart::BSNamePart(Str *name, SrcPos pos, Actuals *params) :
			SimplePart(name, params->values()), pos(pos) {

			exprs = new (this) Array<Expr *>(params->expressions);
		}

		BSNamePart::BSNamePart(const wchar *name, SrcPos pos, Actuals *params) :
			SimplePart(new (this) Str(name), params->values()), pos(pos) {

			exprs = new (this) Array<Expr *>(params->expressions);
		}

		void BSNamePart::insert(Value first) {
			params->insert(0, first);
			exprs->insert(0, new (this) DummyExpr(pos, first));
		}

		void BSNamePart::insert(Value first, Nat at) {
			params->insert(at, first);
			exprs->insert(at, new (this) DummyExpr(pos, first));
		}

		void BSNamePart::alter(Nat at, Value to) {
			params->at(at) = to;
			exprs->at(at) = new (this) DummyExpr(pos, to);
		}

		// TODO: Consider using 'max' for match weights instead?
		Int BSNamePart::matches(Named *candidate) const {
			Array<Value> *c = candidate->params;
			if (c->count() != params->count())
				return -1;

			// We can convert everything to references!
			for (nat i = 0; i < c->count(); i++)
				c->at(i).ref = false;

			int distance = 0;

			for (nat i = 0; i < c->count(); i++) {
				const Value &formal = c->at(i);
				Expr *actual = exprs->at(i);

				int penalty = castPenalty(actual, formal, candidate->flags);
				if (penalty >= 0)
					distance += penalty;
				else
					return -1;
			}

			return distance;
		}

		Name *bsName(syntax::SStr *name, Actuals *params) {
			return new (params) Name(new (params) BSNamePart(name, params));
		}

		Name *bsName(Str *name, SrcPos pos, Actuals *params) {
			return new (params) Name(new (params) BSNamePart(name, pos, params));
		}

	}
}
