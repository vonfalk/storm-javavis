#include "stdafx.h"
#include "Transform.h"
#include "Exception.h"
#include "Type.h"
#include "Rule.h"
#include "Production.h"

#include "Compiler/Basic/Named.h"
#include "Compiler/Basic/WeakCast.h"
#include "Compiler/Basic/If.h"
#include "Compiler/Basic/For.h"
#include "Compiler/Basic/Operator.h"

#include "Compiler/Lib/Array.h"
#include "Compiler/Lib/Maybe.h"

namespace storm {
	namespace syntax {
		using namespace bs;

		// Call a specific member in Basic Storm.
		static Expr *callMember(Str *name, Expr *me, Expr *param = null) {
			Value type = me->result().type();
			if (type == Value())
				throw InternalError(L"Can not call members of 'void'.");

			Actuals *actual = new (me) Actuals();
			Array<Value> *params = new (me) Array<Value>();
			params->push(thisPtr(type.type));
			actual->add(me);
			if (param) {
				params->push(param->result().type());
				actual->add(param);
			}

			SimplePart *part = new (me) SimplePart(name, params);
			Named *found = type.type->find(part);
			Function *toCall = as<Function>(found);
			if (!toCall)
				throw InternalError(::toS(part) + L" was not found!");

			return new (me) FnCall(me->pos, toCall, actual);
		}

		static Expr *callMember(const SrcPos &pos, Str *name, Expr *me, Expr *param = null) {
			try {
				return callMember(name, me, param);
			} catch (const InternalError &e) {
				throw SyntaxError(pos, e.what());
			}
		}

		static Expr *callMember(const wchar *name, Expr *me, Expr *param = null) {
			return callMember(new (me) Str(name), me, param);
		}

		static Expr *callMember(const SrcPos &pos, const wchar *name, Expr *me, Expr *param = null) {
			return callMember(pos, new (me) Str(name), me, param);
		}

		static syntax::SStr *tfmName(ProductionDecl *decl) {
			return CREATE(syntax::SStr, decl, L"transform", decl->pos);
		}

		static Array<ValParam> *tfmParams(Rule *rule, ProductionType *owner) {
			Array<ValParam> *v = new (rule) Array<ValParam>(*rule->params());
			v->insert(0, ValParam(thisPtr(owner), new (rule) Str(L"this")));
			return v;
		}

		TransformFn::TransformFn(ProductionDecl *decl, ProductionType *owner, Rule *rule, Scope scope)
			: BSRawFn(rule->result(), tfmName(decl), tfmParams(rule, owner), null), scope(scope) {

			lookingFor = new (this) Set<Str *>();

			result = decl->result;
			resultParams = decl->resultParams;
			source = owner->production;

			tokenParams = new (this) Array<Params>(decl->tokens->count());
			for (Nat i = 0; i < decl->tokens->count(); i++) {
				TokenDecl *token = decl->tokens->at(i);
				if (RuleTokenDecl *ruleToken = as<RuleTokenDecl>(token)) {
					tokenParams->at(i).v = ruleToken->params;
				}
			}

			// The capture token is treated as being at the end of the token list. Add it if it
			// exists (it never takes any parameters, just like RegexTokens).
			if (source->repCapture)
				tokenParams->push(Params());
		}

		FnBody *TransformFn::createBody() {
			FnBody *root = new (this) FnBody(this, scope);

			Expr *me = createMe(root);

			// Execute members of 'me', or other things that might have side effects.
			executeMe(root, me);

			// Return 'me' if it was declared, unless we return void.
			if (me && Function::result != Value())
				root->add(me);

			// if (wcsncmp(identifier()->c_str(), L"lang.bnf.", 9) == 0)
			// 	PLN(source << L": " << root);
			return root;
		}

		/**
		 * Variables.
		 */

		Expr *TransformFn::createMe(ExprBlock *in) {
			// See if there is a parameter named 'me'. If so: use that!
			for (Nat i = 0; i < params->count(); i++) {
				if (wcscmp(params->at(i).name->c_str(), L"me") == 0) {
					if (result)
						throw SyntaxError(pos, L"Can not use 'me' as a parameter name and specify a result.");
					LocalVar *r = in->variable(new (this) SimplePart(L"me"));
					if (!r)
						throw InternalError(L"Can not find the parameter named 'me'.");
					return new (this) LocalVarAccess(pos, r);
				}
			}

			if (!result)
				return null;

			if (!resultParams) {
				// This is not a function call, just a variable declaration!
				if (result->count() > 1)
					throw SyntaxError(pos, L"The variable " + ::toS(result) + L" is not declared. "
									L"Use " + ::toS(result) + L"() to call it as a function or constructor.");

				return findVar(in, result->at(0)->name);
			}

			Actuals *actual = CREATE(Actuals, this);

			for (Nat i = 0; i < resultParams->count(); i++)
				actual->add(findVar(in, resultParams->at(i)));

			Expr *init = namedExpr(in, pos, result, actual);

			if (init->result() == Value()) {
				// If 'init' returns a void value, do not create a variable.
				in->add(init);
				return null;
			}

			syntax::SStr *name = CREATE(syntax::SStr, this, L"me", pos);
			Var *var = new (this) Var(in, name, init);
			in->add(var);

			return new (this) LocalVarAccess(pos, var->var());

		}

		Expr *TransformFn::readVar(Block *in, Str *name) {
			SimplePart *part = new (this) SimplePart(name);
			if (LocalVar *r = in->variable(part))
				return new (this) LocalVarAccess(pos, r);

			if (*name == L"me")
				throw SyntaxError(pos, L"Can not use 'me' this early!");
			if (*name == L"pos")
				return posVar(in);

			throw InternalError(L"The variable " + ::toS(name) + L" was not created before being read.");
		}

		Expr *TransformFn::findVar(ExprBlock *in, Str *name) {
			if (LocalVar *r = in->variable(new (this) SimplePart(name)))
				return new (this) LocalVarAccess(pos, r);

			if (*name == L"me")
				throw SyntaxError(pos, L"Can not use 'me' this early!");
			if (*name == L"pos")
				return posVar(in);
			if (name->isInt())
				return new (this) Constant(pos, name->toInt());
			if (*name == L"true")
				return new (this) Constant(pos, true);
			if (*name == L"false")
				return new (this) Constant(pos, false);

			// We need to create it...
			if (lookingFor->has(name))
				throw SyntaxError(pos, L"The variable " + ::toS(name) + L" depends on itself.");
			lookingFor->put(name);

			try {
				Nat pos = findToken(name);
				if (pos >= tokenCount())
					throw SyntaxError(this->pos, L"The variable " + ::toS(name) + L" is not declared!");

				LocalVar *var = createVar(in, name, pos);
				lookingFor->remove(name);

				return new (this) LocalVarAccess(this->pos, var);
			} catch (...) {
				lookingFor->remove(name);
				throw;
			}

		}

		Bool TransformFn::hasVar(ExprBlock *in, Str *name) {
			return in->variable(new (this) SimplePart(name)) != null;
		}

		LocalVar *TransformFn::createVar(ExprBlock *in, Str *name, Nat pos) {
			Token *token = getToken(pos);

			if (!token->bound) {
				throw SyntaxError(this->pos, L"The variable " + ::toS(name) + L" is not bound!");
			} else if (token->raw) {
				return createPlainVar(in, name, token);
			} else {
				return createTfmVar(in, name, token, pos);
			}
		}

		LocalVar *TransformFn::createPlainVar(ExprBlock *in, Str *name, Token *token) {
			MemberVar *src = token->target;
			Expr *srcAccess = new (this) MemberVarAccess(pos, thisVar(in), src);

			syntax::SStr *sName = CREATE(syntax::SStr, this, name);
			Var *v = new (this) Var(in, sName, srcAccess);

			in->add(v);

			return v->var();
		}

		LocalVar *TransformFn::createTfmVar(ExprBlock *in, Str *name, Token *token, Nat pos) {
			Engine &e = engine();

			Type *srcType = tokenType(token);
			MemberVar *src = token->target;
			MemberVarAccess *srcAccess = new (this) MemberVarAccess(this->pos, thisVar(in), src);

			// Prepare parameters.
			Actuals *actuals = createActuals(in, pos);
			Function *tfmFn = findTransformFn(srcType, actuals);
			syntax::SStr *varName = new (e) syntax::SStr(name, this->pos);

			// Transform each part of this rule...
			Var *v = null;
			if (isMaybe(src->type)) {
				v = new (this) Var(in, wrapMaybe(tfmFn->result), varName, new (this) Actuals());
				in->add(v);
				Expr *readV = new (this) LocalVarAccess(this->pos, v->var());

				WeakCast *cast = new (this) WeakMaybeCast(srcAccess);
				IfWeak *check = new (this) IfWeak(in, cast);
				assert(check->overwrite(), L"Weak cast should overwrite stuff!");
				Expr *access = new (this) LocalVarAccess(this->pos, check->overwrite());

				// Assign something to the variable!
				IfTrue *branch = new (this) IfTrue(this->pos, check);
				actuals->addFirst(access);
				Expr *tfmCall = new (this) FnCall(this->pos, tfmFn, actuals);
				OpInfo *assignOp = assignOperator(new (e) syntax::SStr(L"="), 1);
				branch->set(new (this) Operator(branch, readV, assignOp, tfmCall));
				check->trueExpr(branch);
				in->add(check);

			} else if (isArray(src->type)) {
				v = new (this) Var(in, wrapArray(tfmFn->result), varName, new (this) Actuals());
				in->add(v);
				Expr *readV = new (this) LocalVarAccess(this->pos, v->var());

				// Extra block to avoid name collisions.
				ExprBlock *forBlock = new (this) ExprBlock(this->pos, in);
				Expr *arrayCount = callMember(L"count", srcAccess);
				Var *end = new (this) Var(forBlock,
										Value(StormInfo<Nat>::type(e)),
										new (e) syntax::SStr(L"_end"),
										arrayCount);
				Expr *readEnd = new (this) LocalVarAccess(this->pos, end->var());
				forBlock->add(end);

				Var *i = new (this) Var(forBlock,
										Value(StormInfo<Nat>::type(e)),
										new (e) syntax::SStr(L"_i"),
										new (this) Constant(this->pos, 0));
				Expr *readI = new (this) LocalVarAccess(this->pos, i->var());
				forBlock->add(i);

				For *loop = new (this) For(this->pos, forBlock);
				loop->test(callMember(L"<", readI, readEnd));
				loop->update(callMember(L"++*", readI));

				actuals->addFirst(callMember(L"[]", srcAccess, readI));
				Expr *tfmCall = new (this) FnCall(this->pos, tfmFn, actuals);
				loop->body(callMember(L"push", readV, tfmCall));

				forBlock->add(loop);
				in->add(forBlock);

			} else {
				// Add 'this' parameter for the call.
				actuals->addFirst(srcAccess);

				// Call function and create variable!
				FnCall *tfmCall = new (this) FnCall(this->pos, tfmFn, actuals);
				v = new (this) Var(in, varName, tfmCall);
				in->add(v);
			}

			return v->var();
		}


		/**
		 * Member function calling.
		 */

		void TransformFn::executeMe(ExprBlock *in, Expr *me) {
			Nat tokens = tokenCount();
			Nat firstBreak = min(source->repStart, tokens);
			Nat secondBreak = min(source->repEnd, tokens);

			// Resolve variables needed for execute steps.
			for (Nat i = 0; i < tokens; i++)
				executeLoad(in, getToken(i), i);

			// Tokens before the capture starts.
			for (Nat i = 0; i < firstBreak; i++)
				executeToken(in, me, getToken(i), i);

			// Tokens in the capture.
			switch (source->repType) {
			case repZeroOne:
				// Only Maybe<T>. If statement is sufficient!
				for (Nat i = firstBreak; i < secondBreak; i++)
					executeTokenIf(in, me, getToken(i), i);
				break;
			case repOnePlus:
			case repZeroPlus:
				// Only Array<T>. Embed inside for-loop.
				executeTokenLoop(firstBreak, secondBreak, in, me);
				break;
			default:
				// Nothing special!
				for (Nat i = firstBreak; i < secondBreak; i++)
					executeToken(in, me, getToken(i), i);
				break;
			}

			// Tokens after the capture.
			for (Nat i = secondBreak; i < tokens; i++)
				executeToken(in, me, getToken(i), i);
		}

		Bool TransformFn::shallExecute(ExprBlock *in, Token *token, Nat pos) {
			// If a token is not even stored in the syntax tree, there is not much we can do...
			if (!token->target)
				return false;

			// Always execute tokens that invoke things.
			if (token->invoke)
				return true;

			// If it is a raw token, there are no side effects to speak about...
			if (token->raw)
				return false;

			// If the token was bound to a variable, only execute it if we have not already done so.
			if (token->bound)
				if (hasVar(in, token->target->name))
					return false;

			// If we got this far, the previous step decided that this token was interesting to
			// evaluate even if it is of no immediate use in this function. Respect that decision
			// and evaluate the token!
			return true;
		}

		void TransformFn::executeToken(ExprBlock *in, Expr *me, Token *token, Nat pos) {
			// Not a token that invokes things.
			if (!shallExecute(in, token, pos))
				return;

			Expr *srcAccess = new (this) MemberVarAccess(this->pos, thisVar(in), token->target);
			in->add(executeToken(in, me, srcAccess, token, pos));
		}

		Expr *TransformFn::executeToken(Block *in, Expr *me, Expr *src, Token *token, Nat pos) {
			Expr *toStore = null;
			if (token->raw) {
				toStore = src;
			} else {
				Type *srcType = tokenType(token);
				Actuals *actuals = readActuals(in, pos);
				Function *tfmFn = findTransformFn(srcType, actuals);

				actuals->addFirst(src);

				toStore = new (this) FnCall(this->pos, tfmFn, actuals);
			}

			if (token->invoke) {
				// Call the member function indicated in 'invoke'.
				if (!me)
					throw SyntaxError(this->pos, L"Can not invoke functions on 'me' when 'me' is undefined.");
				return callMember(this->pos, token->invoke, me, toStore);
			} else {
				// We just called 'transform' for the side effects.
				return toStore;
			}
		}

		void TransformFn::executeTokenIf(ExprBlock *in, Expr *me, Token *token, Nat pos) {
			if (!shallExecute(in, token, pos))
				return;

			Expr *srcAccess = new (this) MemberVarAccess(this->pos, thisVar(in), token->target);
			WeakCast *cast = new (this) WeakMaybeCast(srcAccess);
			IfWeak *check = new (this) IfWeak(in, cast);
			IfTrue *trueBlock = new (this) IfTrue(this->pos, check);

			LocalVar *overwrite = check->overwrite();
			assert(overwrite, L"Weak cast did not overwrite variable as expected.");
			Expr *e = executeToken(trueBlock, me, new (this) LocalVarAccess(this->pos, overwrite), token, pos);
			trueBlock->set(e);
			check->trueExpr(trueBlock);
			in->add(check);
		}

		void TransformFn::executeTokenLoop(Nat from, Nat to, ExprBlock *in, Expr *me) {
			Engine &e = engine();

			// Find the minimal length to iterate...
			Expr *minExpr = null;
			for (Nat i = from; i < to; i++) {
				Token *t = getToken(i);
				if (!shallExecute(in, t, i))
					// Not interesting...
					continue;

				MemberVarAccess *srcAccess = new (this) MemberVarAccess(this->pos, thisVar(in), t->target);
				Expr *read = callMember(L"count", srcAccess);

				if (minExpr)
					minExpr = callMember(L"min", minExpr, read);
				else
					minExpr = read;
			}

			if (!minExpr)
				// Nothing to execute in here!
				return;

			ExprBlock *forBlock = new (this) ExprBlock(this->pos, in);

			Var *end = new (this) Var(forBlock,
									Value(StormInfo<Nat>::type(e)),
									new (e) syntax::SStr(L"_end"),
									minExpr);
			forBlock->add(end);
			Expr *readEnd = new (this) LocalVarAccess(this->pos, end->var());

			// Iterate...
			Var *i = new (this) Var(forBlock,
									Value(StormInfo<Nat>::type(e)),
									new (e) syntax::SStr(L"_i"),
									new (this) Constant(this->pos, 0));
			Expr *readI = new (this) LocalVarAccess(this->pos, i->var());
			forBlock->add(i);

			For *loop = new (this) For(this->pos, forBlock);
			loop->test(callMember(L"<", readI, readEnd));
			loop->update(callMember(L"++*", readI));

			ExprBlock *inLoop = new (this) ExprBlock(this->pos, loop);

			for (Nat i = from; i < to; i++) {
				Token *t = getToken(i);
				if (!shallExecute(in, t, i))
					continue;

				MemberVarAccess *srcAccess = new (this) MemberVarAccess(this->pos, thisVar(in), t->target);
				Expr *element = callMember(L"[]", srcAccess, readI);
				inLoop->add(executeToken(inLoop, me, element, t, i));
			}

			loop->body(inLoop);
			forBlock->add(loop);
			in->add(forBlock);
		}

		void TransformFn::executeLoad(bs::ExprBlock *in, Token *token, Nat pos) {
			// Just requesting the parameters object is enough.
			createActuals(in, pos);
		}



		/**
		 * Utilities.
		 */

		Type *TransformFn::tokenType(Token *token) {
			Value v = token->target->type;
			if (isMaybe(v)) {
				return unwrapMaybe(v).type;
			} else if (isArray(v)) {
				return unwrapArray(v).type;
			} else {
				return v.type;
			}
		}

		Expr *TransformFn::thisVar(Block *in) {
			LocalVar *var = in->variable(new (this) SimplePart(new (this) Str(L"this")));
			assert(var, L"'this' was not found!");
			return new (this) LocalVarAccess(pos, var);
		}

		Expr *TransformFn::posVar(Block *in) {
			Rule *rule = source->rule();

			SimplePart *part = new (this) SimplePart(new (this) Str(L"pos"), thisPtr(rule));
			Named *found = rule->find(part);

			MemberVar *posVar = as<MemberVar>(found);
			assert(posVar, L"'pos' not found in syntax node types!");

			return new (this) MemberVarAccess(pos, thisVar(in), posVar);
		}

		Nat TransformFn::findToken(Str *name) {
			Nat count = tokenCount();

			for (Nat i = 0; i < count; i++) {
				Token *token = getToken(i);

				if (token->invoke)
					// Not interesting.
					continue;

				if (!token->target)
					// Not bound to anything.
					continue;

				if (token->target->name->equals(name))
					return i;
			}

			// Failed.
			return count;
		}

		Token *TransformFn::getToken(Nat pos) {
			Array<Token *> *tokens = source->tokens;
			if (pos == tokens->count())
				return source->repCapture;
			else
				return tokens->at(pos);
		}

		Nat TransformFn::tokenCount() {
			Nat r = source->tokens->count();
			if (source->repCapture)
				r++;
			return r;
		}

		Actuals *TransformFn::readActuals(Block *in, Nat n) {
			Array<Str *> *params = tokenParams->at(n).v;
			Actuals *actuals = new (this) Actuals();

			if (params) {
				for (Nat i = 0; i < params->count(); i++)
					actuals->add(readVar(in, params->at(i)));
			}

			return actuals;
		}

		Actuals *TransformFn::createActuals(ExprBlock *in, Nat n) {
			Array<Str *> *params = tokenParams->at(n).v;
			Actuals *actuals = new (this) Actuals();

			if (params) {
				for (Nat i = 0; i < params->count(); i++)
					actuals->add(findVar(in, params->at(i)));
			}

			return actuals;
		}

		Function *TransformFn::findTransformFn(Type *type, Actuals *actuals) {
			Array<Value> *types = actuals->values();
			types->insert(0, thisPtr(type));

			SimplePart *tfmName = new (this) SimplePart(new (this) Str(L"transform"), types);
			Named *foundTfm = type->find(tfmName);
			Function *tfmFn = as<Function>(foundTfm);
			if (!tfmFn) {
				StrBuf *to = new (this) StrBuf();
				*to << L"Can not transform a " << type->identifier() << L" with parameters: (";
				join(to, actuals->values(), L", ");
				*to << L").";
				throw SyntaxError(pos, to->toS()->c_str());
			}
			return tfmFn;
		}


		/**
		 * Static helpers.
		 */

		Function *createTransformFn(ProductionDecl *decl, ProductionType *type, Scope scope) {
			return new (decl) TransformFn(decl, type, type->rule(), scope);
		}

	}
}
