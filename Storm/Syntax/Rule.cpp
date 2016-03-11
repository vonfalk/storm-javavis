#include "stdafx.h"
#include "Rule.h"
#include "TypeCtor.h"
#include "TypeDtor.h"
#include "Thread.h"
#include "CodeGen.h"
#include "Exception.h"

namespace storm {
	namespace syntax {

		static void CODECALL throwError(Object *me);

		Rule::Rule(Par<RuleDecl> decl, Scope scope) :
			Type(decl->name->v, typeClass),
			pos(decl->pos),
			scope(scope),
			decl(decl),
			tfmNames(decl->paramNames) {

			// Always run on the compiler thread for now.
			setThread(Compiler::decl.threadName(engine));
		}

		void Rule::initTypes() {
			// Already done?
			if (!decl)
				return;

			tfmParams = CREATE(Array<Value>, this);

			// Resolve our parameters...
			for (nat i = 0; i < decl->paramTypes->count(); i++) {
				Value v = scope.value(decl->paramTypes->at(i), decl->pos);
				if (v == Value())
					throw SyntaxError(decl->pos, L"Rules can not take void as a parameter.");
				tfmParams->push(v);
			}

			// Return type...
			tfmResult = scope.value(decl->result, decl->pos);

			decl = null;
		}

		void Rule::clear() {
			// We need to de-allocate anything that could cause trouble when we're destroyed (as
			// we're destroyed very late).
			tfmParams = null;
			tfmNames = null;
			decl = null;
			scope = Scope();

			Type::clear();
		}

		bool Rule::loadAll() {
			initTypes();

			Engine &e = engine;
			Value me = Value::thisPtr(this);

			// Position in the source this node was instantiated.
			add(steal(CREATE(TypeVar, this, this, Value(SrcPos::stormType(e)), L"pos")));

			// Transform function. This should be overloaded by all options.
			Auto<FnPtr<CodeGen *>> tfm = memberWeakPtr(e, this, &Rule::createTransform);
			vector<Value> params = tfmParams->toVector();
			params.insert(params.begin(), me);
			add(steal(lazyFunction(e, tfmResult, L"transform", params, tfm)));

			// This should be private later on.
			throwFn = nativeFunction(e, Value(), L"throwError", valList(1, me), address(&throwError));
			add(throwFn);

			// Add these last.
			add(steal(CREATE(TypeDefaultCtor, this, this)));
			add(steal(CREATE(TypeCopyCtor, this, this)));
			add(steal(CREATE(TypeDeepCopy, this, this)));
			add(steal(CREATE(TypeDefaultDtor, this, this)));

			// Load our functions!
			return Type::loadAll();
		}

		CodeGen *Rule::createTransform() {
			Auto<CodeGen> code = CREATE(CodeGen, this, runOn());
			code::Listing &l = code->to;

			l << code::prolog();

			// Add our this-parameter.
			code::Variable me = code->frame.createPtrParam();

			// Add parameters so we take care of what needs to be taken care of when we throw our exception!
			for (nat i = 0; i < tfmParams->count(); i++)
				code->addParam(tfmParams->at(i), false);

			// Add the return type.
			code->returnType(tfmResult, true);

			// Call our exception-throwing function. It never returns, so we do not have to bother
			// doing proper returns from this function.
			l << code::fnParam(me);
			l << code::fnCall(throwFn->ref(), code::retVoid());

			// Not strictly needed...
			l << code::epilog();
			l << code::ret(code::retVoid());

			return code.ret();
		}

		void CODECALL throwError(Object *me) {
			throw InternalError(L"Can not call 'transform' on a non-derived instance of " + me->myType->identifier());
		}

		Array<Value> *Rule::params() {
			initTypes();

			Auto<Array<Value>> t = tfmParams;
			return t.ret();
		}

		ArrayP<Str> *Rule::names() {
			Auto<ArrayP<Str>> t = tfmNames;
			return t.ret();
		}

		Value Rule::result() {
			initTypes();

			return tfmResult;
		}

	}
}
