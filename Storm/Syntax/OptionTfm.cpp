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

		Expr *callMember(const SrcPos &pos, const String &name, Par<Expr> me, Par<Expr> param = Par<Expr>()) {
			try {
				return callMember(name, me, param);
			} catch (const InternalError &e) {
				throw SyntaxError(pos, e.what());
			}
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

			// The capture token is treated as being at the end of the token list. Add it if it
			// exists (it never takes any parameters, just like RegexTokens).
			if (source->repCapture)
				tokenParams->push(null);
		}

		FnBody *TransformFn::createBody() {
			Auto<FnBody> root = CREATE(FnBody, this, this, scope);

			Auto<Expr> me = createMe(root);

			if (me) {
				// Execute members of 'me'.
				executeMe(root, me);

				// Return 'me' if it was declared.
				root->add(me);
			}

			PVAR(root);
			return root.ret();
		}

		/**
		 * Variables.
		 */

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

		Expr *TransformFn::readVar(Par<Block> in, const String &name) {
			Auto<SimplePart> part = CREATE(SimplePart, this, name);
			if (LocalVar *r = in->variable(part))
				return CREATE(LocalVarAccess, this, steal(r));

			if (name == L"me")
				throw SyntaxError(pos, L"Can not use 'me' this early!");
			if (name == L"pos")
				return posVar(in);

			throw InternalError(L"The variable " + name + L" was not created before being read.");
		}

		Expr *TransformFn::findVar(Par<ExprBlock> in, const String &name) {
			Auto<SimplePart> part = CREATE(SimplePart, this, name);
			if (LocalVar *r = in->variable(part))
				return CREATE(LocalVarAccess, this, steal(r));

			if (name == L"me")
				throw SyntaxError(pos, L"Can not use 'me' this early!");
			if (name == L"pos")
				return posVar(in);

			// We need to create it...
			if (lookingFor.count(name))
				throw SyntaxError(pos, L"The variable " + name + L" depends on itself.");
			lookingFor.insert(name);

			try {
				nat pos = findToken(name);
				if (pos >= tokenCount())
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
			Token *token = getToken(pos);

			if (token->raw) {
				return createPlainVar(in, name, token);
			} else {
				return createTfmVar(in, name, token, pos);
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

		LocalVar *TransformFn::createTfmVar(Par<ExprBlock> in, const String &name, Par<Token> token, nat pos) {
			Engine &e = engine();

			Type *srcType = tokenType(token);
			TypeVar *src = token->target;
			Auto<MemberVarAccess> srcAccess = CREATE(MemberVarAccess, this, steal(thisVar(in)), src);

			// Prepare parameters.
			Auto<Actual> actuals = createActuals(in, pos);
			Function *tfmFn = findTransformFn(srcType, actuals);
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
				Auto<IfTrue> branch = CREATE(IfTrue, this, check);
				actuals->addFirst(access);
				Auto<Expr> tfmCall = CREATE(FnCall, this, tfmFn, actuals);
				Auto<OpInfo> assignOp = assignOperator(sstr(e, L"="), 1);
				branch->set(steal(CREATE(Operator, this, branch, readV, assignOp, tfmCall)));
				check->trueExpr(branch);
				in->add(check);

			} else if (isArray(src->varType)) {
				v = CREATE(Var, this, in, wrapArray(tfmFn->result), varName, steal(CREATE(Actual, this)));
				in->add(v);
				Auto<Expr> readV = CREATE(LocalVarAccess, this, steal(v->var()));

				// Extra block to avoid name collisions.
				Auto<ExprBlock> forBlock = CREATE(ExprBlock, this, in);
				Auto<Var> i = CREATE(Var, this, forBlock, natType(e), sstr(e, L"_i"), steal(CREATE(Constant, this, 0)));
				Auto<Expr> readI = CREATE(LocalVarAccess, this, steal(i->var()));
				forBlock->add(i);

				Auto<For> loop = CREATE(For, this, forBlock);
				Auto<Expr> arrayCount = callMember(L"count", srcAccess);
				loop->test(steal(callMember(L"<", readI, arrayCount)));
				loop->update(steal(callMember(L"++*", readI)));

				actuals->add(steal(callMember(L"[]", srcAccess, readI)));
				Auto<Expr> tfmCall = CREATE(FnCall, this, tfmFn, actuals);
				loop->body(steal(callMember(L"push", readV, tfmCall)));

				forBlock->add(loop);
				in->add(forBlock);

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


		/**
		 * Member function calling.
		 */

		void TransformFn::executeMe(Par<ExprBlock> in, Par<Expr> me) {
			nat tokens = tokenCount();
			nat firstBreak = min(source->repStart, tokens);
			nat secondBreak = min(source->repEnd, tokens);

			// Resolve variables needed for execute steps.
			for (nat i = 0; i < tokens; i++)
				executeLoad(in, getToken(i), i);

			// Tokens before the capture starts.
			for (nat i = 0; i < firstBreak; i++)
				executeToken(in, me, getToken(i), i);

			// Tokens in the capture.
			if (source->repType == repZeroOne()) {
				// Only Maybe<T>. If statement is sufficient!
				for (nat i = firstBreak; i < secondBreak; i++)
					executeTokenIf(in, me, getToken(i), i);
			} else if (source->repType == repOnePlus() || source->repType == repZeroPlus()) {
				// Only Array<T>. Embed inside for-loop.
				executeTokenLoop(firstBreak, secondBreak, in, me);
			} else {
				// Nothing special!
				for (nat i = firstBreak; i < secondBreak; i++)
					executeToken(in, me, getToken(i), i);
			}

			// Tokens after the capture.
			for (nat i = secondBreak; i < tokens; i++)
				executeToken(in, me, getToken(i), i);
		}

		void TransformFn::executeToken(Par<ExprBlock> in, Par<Expr> me, Par<Token> token, nat pos) {
			// Not a token that invokes things.
			if (!token->invoke)
				return;

			Auto<Expr> srcAccess = CREATE(MemberVarAccess, this, steal(thisVar(in)), token->target);
			in->add(steal(executeToken(in, me, srcAccess, token, pos)));
		}

		Expr *TransformFn::executeToken(Par<Block> in, Par<Expr> me, Par<Expr> src, Par<Token> token, nat pos) {
			assert(token->invoke);

			Auto<Expr> toStore;
			if (token->raw) {
				toStore = src;
			} else {
				Type *srcType = tokenType(token);
				Auto<Actual> actuals = readActuals(in, pos);
				Function *tfmFn = findTransformFn(srcType, actuals);

				actuals->addFirst(src);

				toStore = CREATE(FnCall, this, tfmFn, actuals);
			}

		    return callMember(this->pos, token->invoke->v, me, toStore);
		}

		void TransformFn::executeTokenIf(Par<ExprBlock> in, Par<Expr> me, Par<Token> token, nat pos) {
			if (!token->invoke)
				return;

			Auto<Expr> srcAccess = CREATE(MemberVarAccess, this, steal(thisVar(in)), token->target);
			Auto<WeakCast> cast = CREATE(WeakMaybeCast, this, srcAccess);
			Auto<IfWeak> check = CREATE(IfWeak, this, in, cast);
			Auto<IfTrue> trueBlock = CREATE(IfTrue, this, check);

			Auto<LocalVar> overwrite = check->overwrite();
			assert(overwrite, L"Weak cast did not overwrite variable as expected.");
			Auto<Expr> e = executeToken(trueBlock, me, steal(CREATE(LocalVarAccess, this, overwrite)), token, pos);
			trueBlock->set(e);
			check->trueExpr(trueBlock);
			in->add(check);
		}

		void TransformFn::executeTokenLoop(nat from, nat to, Par<ExprBlock> in, Par<Expr> me) {
			Engine &e = engine();

			// Find the minimal length to iterate...
			Auto<Expr> minExpr;
			for (nat i = from; i < to; i++) {
				Token *t = getToken(i);
				if (!t->invoke)
					// Not interesting...
					continue;

				Auto<MemberVarAccess> srcAccess = CREATE(MemberVarAccess, this, steal(thisVar(in)), t->target);
				Auto<Expr> read = callMember(L"count", srcAccess);

				if (minExpr)
					minExpr = callMember(L"min", minExpr, read);
				else
					minExpr = read;
			}

			if (!minExpr)
				// Nothing to execute in here!
				return;

			Auto<ExprBlock> forBlock = CREATE(ExprBlock, this, in);

			Auto<Var> end = CREATE(Var, this, forBlock, natType(e), sstr(e, L"_end"), minExpr);
			forBlock->add(end);
			Auto<Expr> readEnd = CREATE(LocalVarAccess, this, steal(end->var()));

			// Iterate...
			Auto<Var> i = CREATE(Var, this, forBlock, natType(e), sstr(e, L"_i"), steal(CREATE(Constant, this, 0)));
			Auto<Expr> readI = CREATE(LocalVarAccess, this, steal(i->var()));
			forBlock->add(i);

			Auto<For> loop = CREATE(For, this, forBlock);
			loop->test(steal(callMember(L"<", readI, readEnd)));
			loop->update(steal(callMember(L"++*", readI)));

			Auto<ExprBlock> inLoop = CREATE(ExprBlock, this, loop);

			for (nat i = from; i < to; i++) {
				Token *t = getToken(i);
				if (!t->invoke)
					continue;

				Auto<MemberVarAccess> srcAccess = CREATE(MemberVarAccess, this, steal(thisVar(in)), t->target);
				Auto<Expr> element = callMember(L"[]", srcAccess, readI);
				inLoop->add(steal(executeToken(inLoop, me, element, t, i)));
			}

			loop->body(inLoop);
			forBlock->add(loop);
			in->add(forBlock);
		}

		void TransformFn::executeLoad(Par<bs::ExprBlock> in, Par<Token> token, nat pos) {
			// Just requesting the parameters object is enough.
			Auto<Actual> r = createActuals(in, pos);
		}



		/**
		 * Utilities.
		 */

		Type *TransformFn::tokenType(Par<Token> token) {
			const Value &v = token->target->varType;
			if (isMaybe(v)) {
				return unwrapMaybe(v).type;
			} else if (isArray(v)) {
				return unwrapArray(v).type;
			} else {
				return v.type;
			}
		}

		Expr *TransformFn::thisVar(Par<Block> in) {
			Auto<LocalVar> var = in->variable(steal(CREATE(SimplePart, this, L"this")));
			assert(var, L"'this' was not found!");
			return CREATE(LocalVarAccess, this, var);
		}

		Expr *TransformFn::posVar(Par<Block> in) {
			Rule *rule = source->rulePtr();

			Auto<SimplePart> part = CREATE(SimplePart, this, L"pos", valList(1, Value::thisPtr(rule)));
			Auto<Named> found = rule->find(part);

			TypeVar *posVar = as<TypeVar>(found.borrow());
			assert(posVar, L"'pos' not found in syntax node types!");

			return CREATE(MemberVarAccess, this, steal(thisVar(in)), posVar);
		}

		nat TransformFn::findToken(const String &name) {
			nat count = tokenCount();

			for (nat i = 0; i < count; i++) {
				Token *token = getToken(i);

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
			return count;
		}

		Token *TransformFn::getToken(nat pos) {
			ArrayP<Token> *tokens = source->tokens.borrow();
			if (pos == tokens->count())
				return source->repCapture.borrow();
			else
				return tokens->at(pos).borrow();
		}

		nat TransformFn::tokenCount() {
			nat r = source->tokens->count();
			if (source->repCapture)
				r++;
			return r;
		}

		Actual *TransformFn::readActuals(Par<Block> in, nat n) {
			ArrayP<Str> *params = tokenParams->at(n).borrow();
			Auto<Actual> actuals = CREATE(Actual, this);

			if (params) {
				for (nat i = 0; i < params->count(); i++)
					actuals->add(steal(readVar(in, params->at(i)->v)));
			}

			return actuals.ret();
		}

		Actual *TransformFn::createActuals(Par<ExprBlock> in, nat n) {
			ArrayP<Str> *params = tokenParams->at(n).borrow();
			Auto<Actual> actuals = CREATE(Actual, this);

			if (params) {
				for (nat i = 0; i < params->count(); i++)
					actuals->add(steal(findVar(in, params->at(i)->v)));
			}

			return actuals.ret();
		}

		Function *TransformFn::findTransformFn(Par<Type> type, Par<Actual> actuals) {
			vector<Value> types = actuals->values();
			types.insert(types.begin(), Value::thisPtr(type));

			Auto<SimplePart> tfmName = CREATE(SimplePart, this, L"transform", types);
			Auto<Named> foundTfm = type->find(tfmName);
			Function *tfmFn = as<Function>(foundTfm.borrow());
			if (!tfmFn) {
				throw SyntaxError(pos, L"Can not transform a " + ::toS(type->identifier()) +
								L" with parameters: " + join(actuals->values(), L", ") + L".");
			}
			return tfmFn;
		}


		/**
		 * Static helpers.
		 */

		Function *createTransformFn(Par<OptionDecl> decl, Par<OptionType> type, Scope scope) {
			return CREATE(TransformFn, decl, decl, type, type->rulePtr(), scope);
		}

	}
}
