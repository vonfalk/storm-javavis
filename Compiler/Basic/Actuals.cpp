#include "stdafx.h"
#include "Actuals.h"
#include "Cast.h"
#include "Compiler/CodeGen.h"
#include "Core/Join.h"

namespace storm {
	namespace bs {

		Actuals::Actuals() {
			expressions = new (this) Array<Expr *>();
		}

		Actuals::Actuals(Expr *expr) {
			expressions = new (this) Array<Expr *>();
			add(expr);
		}

		Actuals::Actuals(const Actuals &o) : ObjectOn<Compiler>(o) {
			expressions = new (this) Array<Expr *>(*o.expressions);
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
		code::Operand Actuals::code(nat id, CodeGen *s, Value param, Scope scope) {
			using namespace code;

			Expr *expr = castTo(expressions->at(id), param, scope);
			if (param.ref) {
				// If we failed, try to cast to a non-reference type and deal with that later.
				expr = castTo(expressions->at(id), param.asRef(false), scope);
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
				*s->l << lea(tmpRef.v, ptrRel(tmpV.v, Offset()));
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
			*to << S("(") << join(expressions, S(", ")) << S(")");
		}


		BSNamePart::BSNamePart(syntax::SStr *name, Actuals *params) :
			SimplePart(name->v, params->values()), pos(name->pos) {

			exprs = new (this) Array<Expr *>(*params->expressions);
		}

		BSNamePart::BSNamePart(Str *name, SrcPos pos, Actuals *params) :
			SimplePart(name, params->values()), pos(pos) {

			exprs = new (this) Array<Expr *>(*params->expressions);
		}

		BSNamePart::BSNamePart(const wchar *name, SrcPos pos, Actuals *params) :
			SimplePart(new (this) Str(name), params->values()), pos(pos) {

			exprs = new (this) Array<Expr *>(*params->expressions);
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
		// TODO: Consider storing a context inside the context, so that names are resolved in the context
		// they were created rather than in the context they are evaluated. Not sure if this is a good idea though.
		Int BSNamePart::matches(Named *candidate, Scope context) const {
			Array<Value> *c = candidate->params;
			if (c->count() != params->count())
				return -1;

			int distance = 0;

			for (nat i = 0; i < c->count(); i++) {
				// We can convert everything to references, so treat everything as if it was a plain
				// value.
				Value formal = c->at(i).asRef(false);
				Expr *actual = exprs->at(i);

				int penalty = castPenalty(actual, formal, candidate->flags, context);
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
