#include "stdafx.h"
#include "OptionTfm.h"
#include "Parse.h"
#include "Option.h"
#include "Rule.h"
#include "Exception.h"

#include "Lib/Maybe.h"
#include "Lib/ArrayTemplate.h"

#include "Basic/BSVar.h"
#include "Basic/BSNamed.h"
#include "Basic/BSIf.h"
#include "Basic/BSFor.h"
#include "Basic/BSWeakCast.h"
#include "Basic/BSOperator.h"

namespace storm {
	namespace syntax {

		using namespace bs;

		// Call a specific member in Basic Storm.
		Expr *callMember(const String &name, Par<Expr> me, Par<Expr> param = Par<Expr>()) {
			Value type = me->result().type();
			if (type == Value())
				throw InternalError(L"Can not call members of 'void'.");

			Auto<Actual> actual = CREATE(Actual, me);
			vector<Value> params;
			params.push_back(Value::thisPtr(type.type));
			actual->add(me);
			if (param) {
				params.push_back(param->result().type());
				actual->add(param);
			}

			Auto<SimplePart> part = CREATE(SimplePart, me, name, params);
			Auto<Named> found = type.type->find(part);
			Function *toCall = as<Function>(found.borrow());
			if (!toCall)
				throw InternalError(::toS(part) + L" was not found!");

			return CREATE(FnCall, me, toCall, actual);
		}

		static SStr *tfmName(Par<OptionDecl> decl) {
			return CREATE(SStr, decl, L"transform", decl->pos);
		}

		static vector<Value> tfmParams(Par<Rule> rule, Par<OptionType> owner) {
			vector<Value> v = steal(rule->params())->toVector();
			v.insert(v.begin(), Value::thisPtr(owner));
			return v;
		}

		static vector<String> tfmNames(Par<Rule> rule) {
			Auto<ArrayP<Str>> src = rule->names();
			vector<String> result(src->count() + 1);

			result[0] = L"this";
			for (nat i = 0; i < src->count(); i++)
				result[i+1] = src->at(i)->v;

			return result;
		}

		TransformFn::TransformFn(Par<OptionDecl> decl, Par<OptionType> owner, Par<Rule> rule, Scope scope) :
			BSRawFn(rule->result(), steal(tfmName(decl)), tfmParams(rule, owner), tfmNames(rule), null),
			scope(scope) {

			result = decl->result;
			resultParams = decl->resultParams;
			source = owner->option.borrow();

			tokenParams = CREATE(ArrayP<ArrayP<Str>>, this);
			for (nat i = 0; i < decl->tokens->count(); i++) {
				TokenDecl *token = decl->tokens->at(i).borrow();
				if (RuleTokenDecl *ruleToken = as<RuleTokenDecl>(token)) {
					tokenParams->push(ruleToken->params);
				} else {
					tokenParams->push(null);
				}
			}
		}

		FnBody *TransformFn::createBody() {
			Auto<FnBody> root = CREATE(FnBody, this, this, scope);

			Auto<Expr> me = createMe(root);

			if (me) {
				// Execute members of 'me'.
				TODO(L"Execute stuff!");

				// Return 'me' if it was declared.
				root->add(me);
			}

			PVAR(root);
			return root.ret();
		}

		bs::Expr *TransformFn::createMe(Par<ExprBlock> in) {
			if (!result)
				return null;

			if (!resultParams) {
				// This is not a function call, just a variable declaration!
				if (result->count() > 1)
					throw SyntaxError(pos, L"The variable " + ::toS(result) + L" is not declared. "
									L"Use " + ::toS(result) + L"() to call it as a function or constructor.");

				return findVar(in, result->at(0)->name);
			}

			Auto<Actual> actual = CREATE(Actual, this);

			for (nat i = 0; i < resultParams->count(); i++)
				actual->add(steal(findVar(in, resultParams->at(i)->v)));

			Auto<Expr> init = namedExpr(in, pos, result, actual);

			Auto<SStr> name = CREATE(SStr, this, L"me", pos);
			Auto<Var> var = CREATE(Var, this, in, name, init);
			in->add(var);

			return CREATE(LocalVarAccess, this, steal(var->var()));
		}

		Expr *TransformFn::findVar(Par<ExprBlock> in, const String &name) {
			Auto<SimplePart> part = CREATE(SimplePart, this, name);
			if (LocalVar *r = in->variable(part))
				return CREATE(LocalVarAccess, this, steal(r));

			if (name == L"me")
				throw SyntaxError(pos, L"Can not use 'me' this early!");
			// TODO: Support for 'pos'.

			// We need to create it...
			if (lookingFor.count(name))
				throw SyntaxError(pos, L"The variable " + name + L" depends on itself.");
			lookingFor.insert(name);

			try {
				nat pos = findToken(name);
				if (pos >= tokenParams->count())
					throw SyntaxError(this->pos, L"The variable " + name + L" is not declared!");

				Auto<LocalVar> var = createVar(in, name, pos);
				lookingFor.erase(name);

				return CREATE(LocalVarAccess, this, var);
			} catch (...) {
				lookingFor.erase(name);
				throw;
			}

		}

		LocalVar *TransformFn::createVar(Par<ExprBlock> in, const String &name, nat pos) {
			Token *token = source->tokens->at(pos).borrow();

			if (RegexToken *regex = as<RegexToken>(token)) {
				return createPlainVar(in, name, regex);
			} else if (RuleToken *rule = as<RuleToken>(token)) {
				return createRuleVar(in, name, rule, pos);
			} else {
				throw InternalError(L"Unknown subtype of Token found: " + token->myType->identifier());
			}
		}

		LocalVar *TransformFn::createPlainVar(Par<ExprBlock> in, const String &name, Par<Token> token) {
			TypeVar *src = token->target;
			Auto<Expr> srcAccess = CREATE(MemberVarAccess, this, steal(thisVar(in)), src);

			Auto<SStr> sName = CREATE(SStr, this, name);
			Auto<Var> v = CREATE(Var, this, in, sName, srcAccess);

			in->add(v);

			return v->var();
		}

		LocalVar *TransformFn::createRuleVar(Par<ExprBlock> in, const String &name, Par<RuleToken> token, nat pos) {
			Engine &e = engine();

			TypeVar *src = token->target;
			ArrayP<Str> *params = tokenParams->at(pos).borrow();
			Auto<MemberVarAccess> srcAccess = CREATE(MemberVarAccess, this, steal(thisVar(in)), src);

			// Prepare parameters.
			Auto<Actual> actuals = CREATE(Actual, this);
			if (params) {
				for (nat i = 0; i < params->count(); i++)
					actuals->add(steal(findVar(in, params->at(i)->v)));
			}

			// Find the transform function to call on this rule.
			vector<Value> types = actuals->values();
			types.insert(types.begin(), Value::thisPtr(token->rule));

			Auto<SimplePart> tfmName = CREATE(SimplePart, this, L"transform", types);
			Auto<Named> foundTfm = token->rule->find(tfmName);
			Function *tfmFn = as<Function>(foundTfm.borrow());
			if (!tfmFn) {
				throw SyntaxError(this->pos, L"Can not transform a " + ::toS(token->rule->identifier()) +
								L" with parameters: " + join(actuals->values(), L", ") + L".");
			}

			Auto<SStr> varName = sstr(e, name, this->pos);

			// Transform each part of this rule...
			Auto<Var> v;
			if (isMaybe(src->varType)) {
				v = CREATE(Var, this, in, wrapMaybe(tfmFn->result), varName, steal(CREATE(Actual, this)));
				in->add(v);
				Auto<Expr> readV = CREATE(LocalVarAccess, this, steal(v->var()));

				Auto<WeakCast> cast = CREATE(WeakMaybeCast, this, srcAccess);
				Auto<IfWeak> check = CREATE(IfWeak, this, in, cast);
				assert(steal(check->overwrite()), L"Weak cast should overwrite stuff!");
				Auto<Expr> access = CREATE(LocalVarAccess, this, steal(check->overwrite()));

				// Assign something to the variable!
				Auto<IfTrue> branch = CREATE(IfTrue, this, in);
				actuals->addFirst(access);
				Auto<Expr> tfmCall = CREATE(FnCall, this, tfmFn, actuals);
				Auto<OpInfo> assignOp = assignOperator(sstr(e, L"="), 1);
				branch->set(steal(CREATE(Operator, this, branch, readV, assignOp, tfmCall)));
				check->trueExpr(branch);
				in->add(check);

				//assert(false, L"Not implemented yet!");
			} else if (isArray(src->varType)) {
				v = CREATE(Var, this, in, wrapArray(tfmFn->result), varName, steal(CREATE(Actual, this)));
				in->add(v);
				Auto<Expr> readV = CREATE(LocalVarAccess, this, steal(v->var()));

				Auto<Var> i = CREATE(Var, this, in, natType(e), sstr(e, L"i"), steal(CREATE(Constant, this, 0)));
				Auto<Expr> readI = CREATE(LocalVarAccess, this, steal(i->var()));
				in->add(i);

				Auto<For> loop = CREATE(For, this, in);
				Auto<Expr> arrayCount = callMember(L"count", srcAccess);
				loop->test(steal(callMember(L"<", readI, arrayCount)));
				loop->update(steal(callMember(L"++*", readI)));

				actuals->add(steal(callMember(L"[]", srcAccess, readI)));
				Auto<Expr> tfmCall = CREATE(FnCall, this, tfmFn, actuals);
				loop->body(steal(callMember(L"push", readV, tfmCall)));

				in->add(loop);

			} else {
				// Add 'this' parameter for the call.
				actuals->addFirst(srcAccess);

				// Call function and create variable!
				Auto<FnCall> tfmCall = CREATE(FnCall, this, tfmFn, actuals);
				v = CREATE(Var, this, in, varName, tfmCall);
				in->add(v);
			}

			return v->var();
		}

		Expr *TransformFn::thisVar(Par<ExprBlock> in) {
			Auto<LocalVar> var = in->variable(steal(CREATE(SimplePart, this, L"this")));
			assert(var, L"'this' was not found!");
			return CREATE(LocalVarAccess, this, var);
		}

		nat TransformFn::findToken(const String &name) {
			for (nat i = 0; i < source->tokens->count(); i++) {
				Token *token = source->tokens->at(i).borrow();

				if (token->invoke)
					// Not interesting.
					continue;

				if (!token->target)
					// Not bound to anything.
					continue;

				if (token->target->name == name)
					return i;
			}

			// Failed.
			return source->tokens->count();
		}


		Function *createTransformFn(Par<OptionDecl> decl, Par<OptionType> type, Scope scope) {
			return CREATE(TransformFn, decl, decl, type, type->rulePtr(), scope);
		}

	}
}
