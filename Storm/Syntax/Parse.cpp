#include "stdafx.h"
#include "Parse.h"
#include "Exception.h"
#include "Shared/Io/Url.h"
#include "Shared/Io/Text.h"

namespace storm {
	namespace syntax {

		RuleDecl::RuleDecl(SrcPos pos, Par<Str> name, Par<Name> result) :
			pos(pos), name(name), result(result),
			paramTypes(CREATE(ArrayP<Name>, this)),
			paramNames(CREATE(ArrayP<Str>, this)) {}

		void RuleDecl::output(wostream &to) const {
			to << result << L" " << name << L"(";
			for (nat i = 0; i < min(paramTypes->count(), paramNames->count()); i++) {
				if (i > 0)
					to << L", ";
				to << paramTypes->at(i) << L" " << paramNames->at(i);
			}
			to << L");";
		}

		TokenDecl::TokenDecl() {}

		void TokenDecl::output(wostream &to) const {
			if (store) {
				to << L" " << store;
			} else if (invoke) {
				to << L" -> " << invoke;
			}
		}

		RegexTokenDecl::RegexTokenDecl(Par<Str> regex) : regex(regex) {}

		void RegexTokenDecl::output(wostream &to) const {
			to << '"' << regex << '"';
			TokenDecl::output(to);
		}

		RuleTokenDecl::RuleTokenDecl(Par<Name> rule) : rule(rule) {}

		void RuleTokenDecl::output(wostream &to) const {
			to << rule;
			if (params) {
				to << '(';
				for (Nat i = 0; i < params->count(); i++) {
					if (i > 0)
						to << L", ";
					to << params->at(i);
				}
				to << ')';
			}

			TokenDecl::output(to);
		}

		DelimTokenDecl::DelimTokenDecl() {}

		void DelimTokenDecl::output(wostream &to) const {
			to << ", ";
		}

		OptionDecl::OptionDecl(Par<Name> rule) :
			rule(rule),
			priority(0),
			tokens(CREATE(ArrayP<TokenDecl>, this)),
			repStart(0), repEnd(0) {}

		void OptionDecl::output(wostream &to) const {
			to << rule;
			if (priority != 0)
				to << '[' << priority << ']';

			if (result) {
				to << L" => " << result;
				if (resultParams) {
					to << L"(";
					for (Nat i = 0; i < resultParams->count(); i++) {
						if (i > 0)
							to << L", ";
						to << resultParams->at(i);
					}
					to << L")";
				}
			}

			bool usingRep = false;
			usingRep |= repType != repNone();
			usingRep |= repCapture.borrow() != null;

			to << L" : ";
			bool prevDelim = false;
			for (Nat i = 0; i < tokens->count(); i++) {
				Auto<TokenDecl> token = tokens->at(i);
				bool currentDelim = token.as<DelimTokenDecl>() != null;

				if (usingRep && repEnd == i)
					outputRepEnd(to);

				if (i > 0 && !currentDelim && !prevDelim)
					to << L" - ";

				if (usingRep && repStart == i)
					to << L"(";

				to << token;

				prevDelim = currentDelim;
			}

			if (usingRep && repEnd == tokens->count())
				outputRepEnd(to);

			if (name) {
				to << L" = " << name;
			}

			to << L";";
		}

		void OptionDecl::outputRepEnd(wostream &to) const {
			to << ')';

			if (repType == repZeroOne()) {
				to << '?';
			} else if (repType == repOnePlus()) {
				to << '+';
			} else if (repType == repZeroPlus()) {
				to << '*';
			} else if (repCapture) {
				to << repCapture;
			}
		}

		Contents::Contents() :
			rules(CREATE(ArrayP<RuleDecl>, this)),
			options(CREATE(ArrayP<OptionDecl>, this)) {}

		void Contents::output(wostream &to) const {
			if (delimiter) {
				to << endl << L"delimiter = " << delimiter;
			}

			for (Nat i = 0; i < rules->count(); i++) {
				to << endl << rules->at(i);
			}

			for (Nat i = 0; i < options->count(); i++) {
				to << endl << options->at(i);
			}
		}


		/**
		 * Simple parser with 1 token lookahead for parsing the syntax files.
		 * Ident = String of characters (ie. a part of a Name).
		 */

		// Parses a name, eg foo.bar or foo<bar>.baz
		static Name *parseName(Tokenizer &tok, Engine &e) {
			Auto<Name> result = CREATE(Name, e);
			bool first = true;

			do {
				// Skip the previous dot.
				if (!first)
					tok.skip();
				first = false;

				Token id = tok.next();
				vector<Value> params;

				if (tok.peek().token == L"<") {
					throw SyntaxError(tok.position(), L"The <> syntax is not implemented yet!");
				}

				result->add(id.token, params);

			} while (tok.peek().token == L".");

			return result.ret();
		}

		// Parses delimiter = X:
		static Name *parseDelimiter(Tokenizer &tok, Engine &e, Auto<Name> lhs) {
			if (lhs->size() == 1 && lhs->at(0)->name == L"delimiter" && lhs->at(0)->params.size() == 0) {
				tok.skip();
				return parseName(tok, e);
			}

			throw SyntaxError(tok.position(), L"The keyword 'delimiter' should preceed the '=' sign in this context.");
		}

		// Parses a rule declaration.
		static RuleDecl *parseRule(Tokenizer &tok, Engine &e, Auto<Name> resType) {
			Token name = tok.next();

			Auto<Str> nameStr = CREATE(Str, e, name.token);
			Auto<RuleDecl> result = CREATE(RuleDecl, e, name.pos, nameStr, resType);

			tok.expect(L"(");

			while (tok.peek().token != L")") {
				if (tok.peek().token == L",")
					tok.skip();

				Auto<Name> type = parseName(tok, e);
				Auto<Str> name = CREATE(Str, e, tok.next().token);
				result->paramTypes->push(type);
				result->paramNames->push(name);
			}

			tok.expect(L")");

			return result.ret();
		}

		// Is this token a token separator?
		static bool isTokenSep(const Token &t) {
			return t.token == L","
				|| t.token == L"-"
				|| t.token == L";"
				|| t.token == L"="
				|| t.token == L"("
				|| t.token == L")";
		}

		// Parses a parameter list (actual parameters).
		static ArrayP<Str> *parseActuals(Tokenizer &tok, Engine &e) {
			if (tok.peek().token != L"(")
				return null;
			tok.skip();

			Auto<ArrayP<Str>> result = CREATE(ArrayP<Str>, e);

			while (tok.peek().token != L")") {
				if (tok.peek().token == L",")
					tok.skip();

				Auto<Str> s = CREATE(Str, e, tok.next().token);
				result->push(s);
			}

			tok.expect(L")");
			return result.ret();
		}

		// Parses a token inside an option.
		static TokenDecl *parseToken(Tokenizer &tok, Engine &e) {
			Token first = tok.peek();
			Auto<TokenDecl> result;

			if (first.token == L"-") {
				// Simple delimiter, ignore.
				tok.skip();
				return null;
			} else if (first.token == L",") {
				// The delimiter token. May not be bound to anything.
				tok.skip();
				return CREATE(DelimTokenDecl, e);
			} else if (first.isStr()) {
				// Regex.
				tok.skip();
				Auto<Str> regex = CREATE(Str, e, first.strVal());
				result = CREATE(RegexTokenDecl, e, regex);
			} else {
				// Should be the name of something else.
				Auto<Name> rule = parseName(tok, e);
				Auto<RuleTokenDecl> r = CREATE(RuleTokenDecl, e, rule);

				// Any parameters?
				r->params = parseActuals(tok, e);

				result = r;
			}

			Token bind = tok.peek();
			if (isTokenSep(bind))
				return result.ret();

			if (bind.token == L"->") {
				tok.skip();
				bind = tok.next();
				if (isTokenSep(bind))
					throw SyntaxError(bind.pos, L"Expected identifier.");
				result->invoke = CREATE(Str, e, bind.token);
			} else {
				tok.skip();
				result->store = CREATE(Str, e, bind.token);
			}

			return result.ret();
		}

		// Parses an option declaration, from the :.
		static OptionDecl *parseOption(Tokenizer &tok, Engine &e, Auto<Name> member) {
			tok.expect(L":");

			Auto<OptionDecl> result = CREATE(OptionDecl, e, member);

			String lastToken = tok.peek().token;
			while (lastToken != L";" && lastToken != L"=") {
				if (lastToken == L"(") {
					// Start of a capture/repeat!
					result->repStart = result->tokens->count();
					tok.skip();
				} else if (lastToken == L")") {
					// End of a capture/repeat!
					result->repEnd = result->tokens->count();
					tok.skip();

					// Repeat or capture?
					Token rep = tok.next();
					if (rep.token == L"?") {
						result->repType = repZeroOne();
					} else if (rep.token == L"*") {
						result->repType = repZeroPlus();
					} else if (rep.token == L"+") {
						result->repType = repOnePlus();
					} else if (rep.token == L"->") {
						Auto<TokenDecl> decl = CREATE(TokenDecl, e);
						decl->invoke = CREATE(Str, e, tok.next().token);
						result->repCapture = decl;
					} else {
						Auto<TokenDecl> decl = CREATE(TokenDecl, e);
						decl->store = CREATE(Str, e, rep.token);
						result->repCapture = decl;
					}
				} else {
					Auto<TokenDecl> token = parseToken(tok, e);
					if (token)
						result->tokens->push(token);
				}

				lastToken = tok.peek().token;
			}

			if (lastToken == L"=") {
				tok.skip();
				result->name = CREATE(Str, e, tok.next().token);
			}

			return result.ret();
		}

		// Parses an option delcaration, from the =>.
		static OptionDecl *parseOptionResult(Tokenizer &tok, Engine &e, Auto<Name> member) {
			tok.expect(L"=>");

			Auto<Name> name = parseName(tok, e);
			Auto<ArrayP<Str>> params = parseActuals(tok, e);

			Auto<OptionDecl> result = parseOption(tok, e, member);

			result->result = name;
			result->resultParams = params;

			return result.ret();
		}

		// Parses an option declaration with a priority, starting from the [.
		static OptionDecl *parseOptionPriority(Tokenizer &tok, Engine &e, Auto<Name> member) {
			tok.expect(L"[");
			Int prio = tok.next().token.toInt();
			tok.expect(L"]");

			Token sep = tok.peek();
			Auto<OptionDecl> result;
			if (sep.token == L":") {
				result = parseOption(tok, e, member);
			} else if (sep.token == L"=>") {
				result = parseOptionResult(tok, e, member);
			} else {
				throw SyntaxError(sep.pos, L"Unexpected token: " + sep.token);
			}

			result->priority = prio;
			return result.ret();
		}


		// Parses the entire file.
		static Contents *parseFile(Tokenizer &tok, Engine &e) {
			Auto<Contents> result = CREATE(Contents, e);

			while (tok.more()) {
				Auto<Name> name = parseName(tok, e);

				Token sep = tok.peek();
				if (sep.token == L"=") {
					result->delimiter = parseDelimiter(tok, e, name);
				} else if (sep.token == L":") {
					Auto<OptionDecl> decl = parseOption(tok, e, name);
					result->options->push(decl);
				} else if (sep.token == L"=>") {
					Auto<OptionDecl> decl = parseOptionResult(tok, e, name);
					result->options->push(decl);
				} else if (sep.token == L"[") {
					Auto<OptionDecl> decl = parseOptionPriority(tok, e, name);
					result->options->push(decl);
				} else {
					Auto<RuleDecl> decl = parseRule(tok, e, name);
					result->rules->push(decl);
				}

				tok.expect(L";");
			}

			return result.ret();
		}


		// Entry-point to the parser.
		Contents *parseSyntax(Par<Url> file) {
			Engine &e = file->engine();

			// Read file...
			Auto<Str> content = readAllText(file);
			Tokenizer tok(file, content->v, 0);

			return parseFile(tok, e);
		}

	}
}
