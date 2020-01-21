#include "stdafx.h"
#include "Decl.h"
#include "Core/Str.h"
#include "Core/Join.h"
#include "Core/CloneEnv.h"
#include "Exception.h"

namespace storm {
	namespace syntax {

		/**
		 * FileItem.
		 */

		FileItem::FileItem() {}


		/**
		 * Use declarations.
		 */

		UseDecl::UseDecl(SrcName *pkg) : pkg(pkg) {}

		void UseDecl::deepCopy(CloneEnv *env) {
			cloned(pkg, env);
		}

		void UseDecl::toS(StrBuf *to) const {
			*to << S("use ") << pkg << S(";");
		}


		/**
		 * Delimiters.
		 */

		DelimDecl::DelimDecl(SrcName *token) : token(token) {}

		void DelimDecl::deepCopy(CloneEnv *env) {
			cloned(token, env);
		}

		void DelimDecl::toS(StrBuf *to) const {
			*to << S("delimiter = ") << token << S(";");
		}


		/**
		 * Parameters.
		 */

		ParamDecl::ParamDecl(Name *type, Str *name) : type(type), name(name) {}

		StrBuf &operator <<(StrBuf &to, ParamDecl decl) {
			return to << decl.type << L" " << decl.name;
		}


		/**
		 * Rules.
		 */

		RuleDecl::RuleDecl(SrcPos pos, Str *name, Name *result)
			: pos(pos), name(name), result(result), color(tNone) {
			params = new (this) Array<ParamDecl>();
		}

		void RuleDecl::deepCopy(CloneEnv *env) {
			cloned(pos, env);
			cloned(name, env);
			cloned(result, env);
			cloned(params, env);
		}

		void RuleDecl::toS(StrBuf *to) const {
			*to << result << S(" ") << name << S("(") << join(params, S(", ")) << S(")");

			if (color != tNone)
				*to << S(" #") << storm::syntax::name(engine(), color);
			*to << S(";");
		}

		void RuleDecl::push(Array<ParamDecl> *params) {
			this->params = params;
		}

		void RuleDecl::pushColor(SStr *color) {
			TokenColor c = tokenColor(color->v);
			if (c == tNone)
				throw new (this) SyntaxError(color->pos, S("Expected a color name."));
			this->color = c;
		}


		/**
		 * Tokens.
		 */

		TokenDecl::TokenDecl() : color(tNone) {}

		void TokenDecl::deepCopy(CloneEnv *env) {
			cloned(store, env);
			cloned(invoke, env);
		}

		void TokenDecl::toS(StrBuf *to) const {
			if (raw)
				*to << S("@");
			if (store)
				*to << S(" ") << store;
			if (invoke)
				*to << S(" -> ") << invoke;
			if (color != tNone)
				*to << S(" #") << name(engine(), color);
		}

		void TokenDecl::pushRaw(Str *dummy) {
			raw = true;
		}

		void TokenDecl::pushStore(Str *store) {
			this->store = store;
		}

		void TokenDecl::pushInvoke(Str *invoke) {
			this->invoke = invoke;
		}

		void TokenDecl::pushColor(SStr *color) {
			TokenColor c = tokenColor(color->v);
			if (c == tNone)
				throw new (this) SyntaxError(color->pos, S("Expected a color name."));
			this->color = c;
		}

		Str *unescapeStr(Str *s) {
			return s->unescape(Char('"'));
		}


		/**
		 * Regex tokens.
		 */

		RegexTokenDecl::RegexTokenDecl(Str *regex) : regex(regex) {}

		void RegexTokenDecl::deepCopy(CloneEnv *env) {
			TokenDecl::deepCopy(env);
			cloned(regex, env);
		}

		void RegexTokenDecl::toS(StrBuf *to) const {
			*to << S("\"") << regex->escape(Char('"')) << S("\"");
			TokenDecl::toS(to);
		}


		/**
		 * Rule tokens.
		 */

		RuleTokenDecl::RuleTokenDecl(SrcPos pos, Name *rule) : pos(pos), rule(rule) {}

		RuleTokenDecl::RuleTokenDecl(SrcPos pos, Name *rule, Array<Str *> *params) :
			pos(pos), rule(rule), params(params) {}

		void RuleTokenDecl::deepCopy(CloneEnv *env) {
			TokenDecl::deepCopy(env);
			cloned(pos, env);
			cloned(rule, env);
			cloned(params, env);
		}

		void RuleTokenDecl::toS(StrBuf *to) const {
			*to << rule;
			if (params) {
				*to << S("(");
				join(to, params, S(", "));
				*to << S(")");
			}
			TokenDecl::toS(to);
		}


		/**
		 * Delimiter tokens.
		 */

		DelimTokenDecl::DelimTokenDecl() {}

		void DelimTokenDecl::toS(StrBuf *to) const {
			*to << S(", ");
		}

		SepTokenDecl::SepTokenDecl() {}

		void SepTokenDecl::toS(StrBuf *to) const {
			*to << S(" - ");
		}


		/**
		 * Productions.
		 */

		ProductionDecl::ProductionDecl(SrcPos pos, Name *memberOf) : pos(pos), parent(null), rule(memberOf) {
			tokens = new (this) Array<TokenDecl *>();
			repType = repNone;
			indentType = indentNone;
		}

		ProductionDecl::ProductionDecl(SrcPos pos, Name *memberOf, MAYBE(Name *) parent)
			: pos(pos), parent(parent), rule(memberOf) {

			tokens = new (this) Array<TokenDecl *>();
			repType = repNone;
			indentType = indentNone;
		}

		void ProductionDecl::deepCopy(CloneEnv *env) {
			cloned(pos, env);
			cloned(parent, env);
			cloned(rule, env);
			cloned(tokens, env);
			cloned(name, env);
			cloned(result, env);
			cloned(resultParams, env);
			cloned(repCapture, env);
		}

		void ProductionDecl::toS(StrBuf *to) const {
			if (parent)
				*to << parent << S("..");

			*to << rule;
			if (priority != 0)
				*to << S("[") << priority << S("]");

			if (result) {
				*to << S(" => ") << result;
				if (resultParams) {
					*to << S("(");
					join(to, resultParams, S(", "));
					*to << S(")");
				}
			}

			bool usingRep = false;
			usingRep |= repType != repNone;
			usingRep |= repCapture != null;

			bool usingIndent = indentType != indentNone;

			*to << S(" : ");
			bool prevDelim = false;
			for (Nat i = 0; i < tokens->count(); i++) {
				TokenDecl *token = tokens->at(i);
				bool currentDelim = as<DelimTokenDecl>(token) != null;

				if (usingRep && repEnd == i)
					outputRepEnd(to);

				if (usingIndent && indentEnd == i)
					*to << S(" ]") << indentType;

				if (i > 0 && !currentDelim && !prevDelim)
					*to << S(" - ");

				if (usingIndent && indentStart == i)
					*to << S("[");

				if (usingRep && repStart == i)
					*to << S("(");

				*to << token;

				prevDelim = currentDelim;
			}

			if (usingRep && repEnd == tokens->count())
				outputRepEnd(to);

			if (usingIndent && indentEnd == tokens->count())
				*to << S(" ]") << indentType;

			if (name)
				*to << S(" = ") << name;

			*to << S(";");
		}

		void ProductionDecl::outputRepEnd(StrBuf *to) const {
			*to << S(")");

			switch (repType) {
			case repZeroOne:
				*to << S("?");
				break;
			case repOnePlus:
				*to << S("+");
				break;
			case repZeroPlus:
				*to << S("*");
				break;
			case repNone:
				if (repCapture)
					*to << repCapture;
				break;
			}
		}

		void ProductionDecl::pushPrio(Int prio) {
			priority = prio;
		}

		void ProductionDecl::pushName(Str *name) {
			this->name = name;
		}

		void ProductionDecl::push(TokenDecl *decl) {
			if (as<SepTokenDecl>(decl))
				;
			else
				tokens->push(decl);
		}

		void ProductionDecl::pushResult(Name *result) {
			this->result = result;
		}

		void ProductionDecl::pushResult(Str *result) {
			this->result = new (this) Name(result);
		}

		void ProductionDecl::pushParams(Array<Str *> *p) {
			resultParams = p;
		}

		void ProductionDecl::pushRepStart(Str *dummy) {
			repStart = tokens->count();
		}

		void ProductionDecl::pushRepEnd(RepType type) {
			repEnd = tokens->count();
			repType = type;
		}

		void ProductionDecl::pushRepEnd(TokenDecl *capture) {
			repEnd = tokens->count();
			repType = repNone;
			repCapture = capture;
		}

		void ProductionDecl::pushIndentStart(Str *dummy) {
			indentStart = tokens->count();
		}

		void ProductionDecl::pushIndentEnd(IndentType type) {
			indentEnd = tokens->count();
			indentType = type;
		}


		/**
		 * Custom declaration.
		 */

		CustomDecl::CustomDecl() {}

		void CustomDecl::expand(FileContents *to) {}


		/**
		 * File contents.
		 */

		FileContents::FileContents() {
			use = new (this) Array<SrcName *>();
			rules = new (this) Array<RuleDecl *>();
			productions = new (this) Array<ProductionDecl *>();
		}

		void FileContents::push(FileItem *item) {
			if (RuleDecl *r = as<RuleDecl>(item))
				rules->push(r);
			else if (ProductionDecl *p = as<ProductionDecl>(item))
				productions->push(p);
			else if (UseDecl *u = as<UseDecl>(item))
				use->push(u->pkg);
			else if (DelimDecl *d = as<DelimDecl>(item))
				delimiter = d->token;
			else if (CustomDecl *c = as<CustomDecl>(item))
				c->expand(this);
			else
				WARNING(L"Unknown FileItem!");
		}

		void FileContents::deepCopy(CloneEnv *env) {
			cloned(use, env);
			cloned(delimiter, env);
			cloned(rules, env);
			cloned(productions, env);
		}

		void FileContents::toS(StrBuf *to) const {
			for (nat i = 0; i < use->count(); i++)
				*to << S("\nuse ") << use->at(i);

			if (delimiter)
				*to << S("\ndelimiter = ") << delimiter;

			if (rules->any())
				*to << S("\n");
			for (nat i = 0; i < rules->count(); i++)
				*to << S("\n") << rules->at(i);

			if (productions->any())
				*to << S("\n");
			for (nat i = 0; i < productions->count(); i++)
				*to << S("\n") << productions->at(i);
		}

		Str *joinName(Str *first, Array<Str *> *rest) {
			StrBuf *to = new (first) StrBuf();
			*to << first;

			for (Nat i = 0; i < rest->count(); i++)
				*to << S(".") << rest->at(i);

			return to->toS();
		}

		RuleDecl *applyDoc(SrcPos doc, RuleDecl *decl) {
			decl->docPos = doc;
			return decl;
		}

		ProductionDecl *applyDoc(SrcPos doc, ProductionDecl *decl) {
			decl->docPos = doc;
			return decl;
		}

		FileItem *applyDoc(SrcPos doc, FileItem *decl) {
			if (RuleDecl *rule = as<RuleDecl>(decl))
				applyDoc(doc, rule);
			else if (ProductionDecl *prod = as<ProductionDecl>(decl))
				applyDoc(doc, prod);

			return decl;
		}

	}
}
