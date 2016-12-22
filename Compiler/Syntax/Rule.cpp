#include "stdafx.h"
#include "Rule.h"
#include "Node.h"
#include "Exception.h"
#include "Engine.h"
#include "Core/Str.h"
#include "Compiler/CodeGen.h"
#include "Compiler/Function.h"
#include "Compiler/TypeCtor.h"

namespace storm {
	namespace syntax {


		Rule::Rule(RuleDecl *decl, Scope scope)
			: Type(decl->name, typeClass),
			  pos(decl->pos),
			  scope(scope),
			  decl(decl) {

			setSuper(Node::stormType(engine));
		}

		Bool Rule::loadAll() {
			initTypes();

			Value me = thisPtr(this);

			// Transform function. This should be overloaded by all options.
			Array<Value> *params = new (engine) Array<Value>();
			params->push(me);
			for (nat i = 0; i < tfmParams->count(); i++)
				params->push(tfmParams->at(i).type);

			add(lazyFunction(engine, tfmResult, L"transform", params, fnPtr(engine, &Rule::createTransform, this)));

			// Add these last.
			add(new (this) TypeDefaultCtor(this));
			add(new (this) TypeCopyCtor(this));
			add(new (this) TypeDeepCopy(this));

			return Type::loadAll();
		}

		void Rule::initTypes() {
			// Already done?
			if (!decl)
				return;

			tfmParams = new (this) Array<bs::ValParam>();
			for (nat i = 0; i < decl->params->count(); i++) {
				ParamDecl p = decl->params->at(i);
				Value v = scope.value(p.type, decl->pos);
				if (v == Value())
					throw SyntaxError(decl->pos, L"Rules can not take void as a parameter.");
				tfmParams->push(bs::ValParam(v, p.name));
			}

			tfmResult = scope.value(decl->result, decl->pos);

			decl = null;
		}

		Array<bs::ValParam> *Rule::params() {
			initTypes();
			return tfmParams;
		}

		Value Rule::result() {
			initTypes();
			return tfmResult;
		}

		CodeGen *Rule::createTransform() {
			CodeGen *code = new (this) CodeGen(runOn());
			code::Listing *l = code->to;

			// Add our parameters.
			code::Var me = code->createVar(tfmParams->at(0).type).v;
			for (nat i = 1; tfmParams->count(); i++)
				code->createVar(tfmParams->at(i).type);

			// Add return type.
			code->result(tfmResult, true);

			// Call the exception-throwing function.
			*l << code::fnParam(me);
			*l << code::fnCall(engine.ref(Engine::rRuleThrow), code::valVoid());

			// Not needed, but for good measure.
			*l << code::epilog();
			*l << code::ret(tfmResult.valTypeRet());

			return code;
		}

	}
}
