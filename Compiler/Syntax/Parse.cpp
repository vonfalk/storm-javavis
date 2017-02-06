#include "stdafx.h"
#include "Parse.h"
#include "Tokenizer.h"
#include "Exception.h"
#include "Core/Io/Text.h"
#include "Core/Str.h"

namespace storm {
	namespace syntax {

		// Parses a name, eg foo.bar or foo<bar>.baz.
		static SrcName *parseName(Engine &e, Tokenizer &tok) {
			SrcName *result = new (e) SrcName(tok.position());
			bool first = true;

			do {
				// Skip the previous dot.
				if (!first)
					tok.expect(L".");
				first = false;

				Token id = tok.next();
				if (tok.skipIf(L"<")) {
					Array<Name *> *params = new (e) Array<Name *>();
					while (tok.peek() != L">") {
						tok.skipIf(L",");
						params->push(parseName(e, tok));
					}
					tok.expect(L">");

					result->add(new (e) RecPart(id.toS(), params));
				} else {
					result->add(id.toS());
				}
			} while (tok.peek() == L".");

			return result;
		}

		// See if a token is a separator.
		static bool isTokenSep(const Token &t) {
			return t == L","
				|| t == L"-"
				|| t == L";"
				|| t == L"="
				|| t == L"("
				|| t == L")";
		}

		// Parse a parameter list (actual parameters).
		static Array<Str *> *parseActuals(Engine &e, Tokenizer &tok) {
			if (!tok.skipIf(L"("))
				return null;

			Array<Str *> *r = new (e) Array<Str *>();
			while (tok.peek() != L")") {
				tok.skipIf(L",");
				r->push(tok.next().toS());
			}

			tok.expect(L")");
			return r;
		}

		// Parse a token color.
		static TokenColor parseTokenColor(Tokenizer &tok) {
			TokenColor c = tokenColor(tok.next().toS());
			if (c == tNone)
				throw SyntaxError(tok.position(), L"Expected a color name.");
			return c;
		}

		// Parse a token.
		static TokenDecl *parseToken(Engine &e, Tokenizer &tok) {
			TokenDecl *result = null;

			if (tok.skipIf(L"-")) {
				return null;
			} else if (tok.skipIf(L",")) {
				// The delimiter token. Might not be bound to anything.
				return new (e) DelimTokenDecl();
			} else if (tok.peek().isStrLiteral()) {
				// Regex.
				Token regex = tok.next();
				result = new (e) RegexTokenDecl(regex.strLiteral());
			} else {
				// Should be the name of something else.
				SrcName *rule = parseName(e, tok);
				RuleTokenDecl *r = new (e) RuleTokenDecl(rule->pos, rule);
				r->params = parseActuals(e, tok);
				result = r;
			}

			if (isTokenSep(tok.peek()))
				return result;

			if (tok.skipIf(L"@")) {
				result->raw = true;

				if (isTokenSep(tok.peek()))
					return result;
			}

			if (tok.skipIf(L"->")) {
				if (isTokenSep(tok.peek()))
					throw SyntaxError(tok.position(), L"Expected identifier.");
				result->invoke = tok.next().toS();

				// Maybe a color name as well.
				if (tok.skipIf(L"#"))
					result->color = parseTokenColor(tok);
			} else if (tok.skipIf(L"#")) {
				result->color = parseTokenColor(tok);
			} else {
				// Simple identifier.
				result->store = tok.next().toS();

				// Maybe a color name as well.
				if (tok.skipIf(L"#"))
					result->color = parseTokenColor(tok);
			}

			return result;
		}

		// Parse the bindings of a capture.
		static TokenDecl *parseCapture(Engine &e, Tokenizer &tok, const Token &rep) {
			if (rep == L"->") {
				TokenDecl *decl = new (e) TokenDecl();
				decl->invoke = tok.next().toS();
				return decl;
			} else if (isTokenSep(rep)) {
				throw SyntaxError(rep.pos, L"Expected -> or identifier.");
			} else {
				TokenDecl *decl = new (e) TokenDecl();
				decl->store = rep.toS();
				return decl;
			}
		}

		// Parse a production.
		static ProductionDecl *parseProduction(Engine &e, Tokenizer &tok, SrcName *rule) {
			tok.expect(L":");
			ProductionDecl *result = new (e) ProductionDecl(rule->pos, rule);

			while (tok.peek() != L";" && tok.peek() != L"=") {
				if (tok.skipIf(L"(")) {
					// Start of a capture/repeat!
					result->repStart = result->tokens->count();
				} else if (tok.skipIf(L")")) {
					// End of a capture/repeat!
					result->repEnd = result->tokens->count();

					Token rep = tok.next();
					if (rep == L"?") {
						result->repType = repZeroOne;
					} else if (rep == L"*") {
						result->repType = repZeroPlus;
					} else if (rep == L"+") {
						result->repType = repOnePlus;
					} else if (rep == L"@") {
						rep = tok.next();
						result->repCapture = parseCapture(e, tok, rep);
						result->repCapture->raw = true;
					} else if (isTokenSep(rep)) {
						throw SyntaxError(rep.pos, L"Expected ?, *, +, ->, @ or identifier.");
					} else {
						result->repCapture = parseCapture(e, tok, rep);
					}
				} else {
					TokenDecl *token = parseToken(e, tok);
					if (token)
						result->tokens->push(token);
				}
			}

			if (tok.skipIf(L"="))
				result->name = tok.next().toS();

			return result;
		}

		// Parse a production with a result.
		static ProductionDecl *parseProductionResult(Engine &e, Tokenizer &tok, SrcName *rule) {
			tok.expect(L"=>");

			Name *name = parseName(e, tok);
			Array<Str *> *params = parseActuals(e, tok);

			ProductionDecl *result = parseProduction(e, tok, rule);
			result->result = name;
			result->resultParams = params;

			return result;
		}

		// Parse a production with a priority.
		static ProductionDecl *parseProductionPriority(Engine &e, Tokenizer &tok, SrcName *rule) {
			tok.expect(L"[");
			Int prio = 0;
			try {
				if (tok.skipIf(L"-")) {
					prio = -tok.next().toS()->toInt();
				} else if (tok.skipIf(L"+")) {
					prio = tok.next().toS()->toInt();
				} else {
					prio = tok.next().toS()->toInt();
				}
			} catch (const StrError &e) {
				throw SyntaxError(tok.position(), e.what());
			}
			tok.expect(L"]");

			ProductionDecl *result = null;
			Token sep = tok.peek();
			if (sep == L":") {
				result = parseProduction(e, tok, rule);
			} else if (sep == L"=>") {
				result = parseProductionResult(e, tok, rule);
			} else {
				throw SyntaxError(sep.pos, L"Unexpected token: " + ::toS(sep));
			}

			result->priority = prio;
			return result;
		}

		// Parse a rule declaration.
		static RuleDecl *parseRule(Engine &e, Tokenizer &tok, SrcName *result) {
			Token name = tok.next();
			RuleDecl *r = new (e) RuleDecl(name.pos, name.toS(), result);

			tok.expect(L"(");

			while (tok.peek() != L")") {
				tok.skipIf(L",");
				Name *type = parseName(e, tok);
				Str *name = tok.next().toS();
				r->params->push(ParamDecl(type, name));
			}

			tok.expect(L")");

			return r;
		}

		static FileContents *parseFile(Engine &e, Tokenizer &tok) {
			FileContents *r = new (e) FileContents();

			while (tok.more()) {
				if (tok.skipIf(L"use")) {
					r->use->push(parseName(e, tok));
				} else if (tok.skipIf(L"delimiter")) {
					tok.expect(L"=");
					r->delimiter = parseName(e, tok);
				} else {
					SrcName *name = parseName(e, tok);
					Token sep = tok.peek();

					if (sep == L":") {
						r->productions->push(parseProduction(e, tok, name));
					} else if (sep == L"=>") {
						r->productions->push(parseProductionResult(e, tok, name));
					} else if (sep == L"[") {
						r->productions->push(parseProductionPriority(e, tok, name));
					} else {
						r->rules->push(parseRule(e, tok, name));
					}
				}

				tok.expect(L";");
			}

			return r;
		}

		FileContents *parseSyntax(Str *data, Url *url, Str::Iter start) {
			Tokenizer tok(url, data, start.offset());
			return parseFile(data->engine(), tok);
		}

	}
}
