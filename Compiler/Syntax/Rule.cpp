#include "stdafx.h"
#include "Rule.h"
#include "Node.h"
#include "Doc.h"
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
			  decl(decl),
			  color(decl->color) {

			setSuper(Node::stormType(engine));

			if (decl->docPos.any())
				documentation = new (this) SyntaxDoc(decl->docPos, this);
		}

		Bool Rule::loadAll() {
			initTypes();

			Value me = thisPtr(this);

			// Transform function. This should be overloaded by all options.
			Array<Value> *params = new (engine) Array<Value>();
			params->push(me);
			for (nat i = 0; i < tfmParams->count(); i++)
				params->push(tfmParams->at(i).type);

			add(lazyFunction(engine, tfmResult, S("transform"), params, fnPtr(engine, &Rule::createTransform, this)));

			// Add these last.
			add(new (this) TypeDefaultCtor(this));
			if (!me.isActor()) {
				add(new (this) TypeCopyCtor(this));
				add(new (this) TypeDeepCopy(this));
			}

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
					throw new (this) SyntaxError(decl->pos, S("Rules can not take void as a parameter."));
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
			CodeGen *code = new (this) CodeGen(runOn(), true, tfmResult);
			code::Listing *l = code->l;

			// Add our parameters.
			code::Var me = code->createParam(thisPtr(this));
			for (nat i = 0; i < tfmParams->count(); i++)
				code->createParam(tfmParams->at(i).type);

			*l << code::prolog();

			// Call the exception-throwing function.
			*l << code::fnParam(engine.ptrDesc(), me);
			*l << code::fnCall(engine.ref(Engine::rRuleThrow), true);

			// Not needed, but for good measure.
			*l << code::epilog();
			*l << code::ret(Size());

			return code;
		}

	}
}
