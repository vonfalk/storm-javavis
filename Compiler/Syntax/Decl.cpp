#include "stdafx.h"
#include "Decl.h"
#include "Core/Str.h"
#include "Core/CloneEnv.h"

namespace storm {
	namespace syntax {

		ParamDecl::ParamDecl(Name *type, Str *name) : type(type), name(name) {}

		StrBuf &operator <<(StrBuf &to, ParamDecl decl) {
			return to << decl.type << L" " << decl.name;
		}



		RuleDecl::RuleDecl(SrcPos pos, Str *name, Name *result)
			: pos(pos), name(name), result(result) {
			params = new (this) Array<ParamDecl>();
		}

		void RuleDecl::deepCopy(CloneEnv *env) {
			cloned(pos, env);
			cloned(name, env);
			cloned(result, env);
			cloned(params, env);
		}

		void RuleDecl::toS(StrBuf *to) const {
			*to << result << L" " << name << L"(";
			join(to, params, L", ");
			*to << L");";
		}


		TokenDecl::TokenDecl() : color(tNone) {}

		void TokenDecl::deepCopy(CloneEnv *env) {
			cloned(store, env);
			cloned(invoke, env);
		}

		void TokenDecl::toS(StrBuf *to) const {
			if (raw)
				*to << L"@";
			if (store)
				*to << L" " << store;
			if (invoke)
				*to << L" -> " << invoke;
			if (color != tNone)
				*to << L" #" << name(engine(), color);
		}


		RegexTokenDecl::RegexTokenDecl(Str *regex) : regex(regex) {}

		void RegexTokenDecl::deepCopy(CloneEnv *env) {
			TokenDecl::deepCopy(env);
			cloned(regex, env);
		}

		void RegexTokenDecl::toS(StrBuf *to) const {
			*to << L"\"" << regex->escape(Char('"')) << L"\"";
			TokenDecl::toS(to);
		}


		RuleTokenDecl::RuleTokenDecl(SrcPos pos, Name *rule) : pos(pos), rule(rule) {}

		void RuleTokenDecl::deepCopy(CloneEnv *env) {
			TokenDecl::deepCopy(env);
			cloned(pos, env);
			cloned(rule, env);
			cloned(params, env);
		}

		void RuleTokenDecl::toS(StrBuf *to) const {
			*to << rule;
			if (params) {
				*to << L"(";
				join(to, params, L", ");
				*to << L")";
			}
			TokenDecl::toS(to);
		}


		DelimTokenDecl::DelimTokenDecl() {}

		void DelimTokenDecl::toS(StrBuf *to) const {
			*to << L", ";
		}


		ProductionDecl::ProductionDecl(SrcPos pos, Name *memberOf) : pos(pos), rule(memberOf) {
			tokens = new (this) Array<TokenDecl *>();
		}

		void ProductionDecl::deepCopy(CloneEnv *env) {
			cloned(pos, env);
			cloned(rule, env);
			cloned(tokens, env);
			cloned(name, env);
			cloned(result, env);
			cloned(resultParams, env);
			cloned(repCapture, env);
		}

		void ProductionDecl::toS(StrBuf *to) const {
			*to << rule;
			if (priority != 0)
				*to << L"[" << priority << L"]";

			if (result) {
				*to << L" => " << result;
				if (resultParams) {
					*to << L"(";
					join(to, resultParams, L", ");
					*to << L")";
				}
			}

			bool usingRep = false;
			usingRep |= repType != repNone;
			usingRep |= repCapture != null;

			*to << L" : ";
			bool prevDelim = false;
			for (Nat i = 0; i < tokens->count(); i++) {
				TokenDecl *token = tokens->at(i);
				bool currentDelim = as<DelimTokenDecl>(token) != null;

				if (usingRep && repEnd == i)
					outputRepEnd(to);

				if (i > 0 && !currentDelim && !prevDelim)
					*to << L" - ";

				if (usingRep && repStart == i)
					*to << L"(";

				*to << token;

				prevDelim = currentDelim;
			}

			if (usingRep && repEnd == tokens->count())
				outputRepEnd(to);

			if (name)
				*to << L" = " << name;

			*to << L";";
		}

		void ProductionDecl::outputRepEnd(StrBuf *to) const {
			*to << L")";

			switch (repType) {
			case repZeroOne:
				*to << L"?";
				break;
			case repOnePlus:
				*to << L"+";
				break;
			case repZeroPlus:
				*to << L"*";
				break;
			case repNone:
				if (repCapture)
					*to << repCapture;
				break;
			}
		}


		FileContents::FileContents() {
			use = new (this) Array<SrcName *>();
			rules = new (this) Array<RuleDecl *>();
			productions = new (this) Array<ProductionDecl *>();
		}

		void FileContents::deepCopy(CloneEnv *env) {
			cloned(use, env);
			cloned(delimiter, env);
			cloned(rules, env);
			cloned(productions, env);
		}

		void FileContents::toS(StrBuf *to) const {
			for (nat i = 0; i < use->count(); i++)
				*to << L"\nuse " << use->at(i);

			if (delimiter)
				*to << L"\ndelimiter = " << delimiter;

			if (rules->any())
				*to << L"\n";
			for (nat i = 0; i < rules->count(); i++)
				*to << L"\n" << rules->at(i);

			if (productions->any())
				*to << L"\n";
			for (nat i = 0; i < productions->count(); i++)
				*to << L"\n" << productions->at(i);
		}
	}
}
