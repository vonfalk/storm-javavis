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
					tok.expect(S("."));
				first = false;

				Token id = tok.next();
				if (tok.skipIf(S("<"))) {
					Array<Name *> *params = new (e) Array<Name *>();
					while (tok.peek() != S(">")) {
						tok.skipIf(S(","));
						params->push(parseName(e, tok));
					}
					tok.expect(S(">"));

					result->add(new (e) RecPart(id.toS(), params));
				} else {
					result->add(id.toS());
				}
			} while (tok.peek() == S("."));

			return result;
		}

		// See if a token is a separator.
		static bool isTokenSep(const Token &t) {
			return t == S(",")
				|| t == S("-")
				|| t == S(";")
				|| t == S("=")
				|| t == S("(")
				|| t == S(")")
				|| t == S("[")
				|| t == S("]");
		}

		// Parse a parameter list (actual parameters).
		static Array<Str *> *parseActuals(Engine &e, Tokenizer &tok) {
			if (!tok.skipIf(S("(")))
				return null;

			if (tok.skipIf(S(",")))
				throw new (e) SyntaxError(tok.position(), S("Actual parameters to a token may not start with a comma.")
										S(" If you meant to start a capture, use - to disambiguate."));

			Array<Str *> *r = new (e) Array<Str *>();
			while (tok.peek() != S(")")) {
				tok.skipIf(S(","));
				r->push(tok.next().toS());
			}

			tok.expect(S(")"));
			return r;
		}

		// Parse a token color.
		static TokenColor parseTokenColor(Engine &e, Tokenizer &tok) {
			TokenColor c = tokenColor(tok.next().toS());
			if (c == tNone)
				throw new (e) SyntaxError(tok.position(), S("Expected a color name."));
			return c;
		}

		// Parse a token.
		static TokenDecl *parseToken(Engine &e, Tokenizer &tok) {
			TokenDecl *result = null;

			if (tok.skipIf(S("-"))) {
				return null;
			} else if (tok.skipIf(S(","))) {
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

			if (tok.skipIf(S("@"))) {
				result->raw = true;

				if (isTokenSep(tok.peek()))
					return result;
			}

			if (tok.skipIf(S("->"))) {
				if (isTokenSep(tok.peek()))
					throw new (e) SyntaxError(tok.position(), S("Expected identifier."));
				result->invoke = tok.next().toS();

				// Maybe a color name as well.
				if (tok.skipIf(S("#")))
					result->color = parseTokenColor(e, tok);
			} else if (tok.skipIf(S("#"))) {
				result->color = parseTokenColor(e, tok);
			} else {
				// Simple identifier.
				result->store = tok.next().toS();

				// Maybe a color name as well.
				if (tok.skipIf(S("#")))
					result->color = parseTokenColor(e, tok);
			}

			return result;
		}

		// Parse the bindings of a capture.
		static TokenDecl *parseCapture(Engine &e, Tokenizer &tok, const Token &rep) {
			if (rep == S("->")) {
				TokenDecl *decl = new (e) TokenDecl();
				decl->invoke = tok.next().toS();
				return decl;
			} else if (isTokenSep(rep)) {
				throw new (e) SyntaxError(rep.pos, S("Expected -> or identifier."));
			} else {
				TokenDecl *decl = new (e) TokenDecl();
				decl->store = rep.toS();
				return decl;
			}
		}

		// Parse a production.
		static ProductionDecl *parseProduction(Engine &e, Tokenizer &tok, SrcName *rule) {
			tok.expect(S(":"));
			ProductionDecl *result = new (e) ProductionDecl(rule->pos, rule);

			while (tok.peek() != S(";") && tok.peek() != S("=")) {
				if (tok.skipIf(S("("))) {
					// Start of a capture/repeat!
					result->repStart = result->tokens->count();
				} else if (tok.skipIf(S(")"))) {
					// End of a capture/repeat!
					result->repEnd = result->tokens->count();

					Token rep = tok.next();
					if (rep == S("?")) {
						result->repType = repZeroOne;
					} else if (rep == S("*")) {
						result->repType = repZeroPlus;
					} else if (rep == S("+")) {
						result->repType = repOnePlus;
					} else if (rep == S("@")) {
						rep = tok.next();
						result->repCapture = parseCapture(e, tok, rep);
						result->repCapture->raw = true;
					} else if (isTokenSep(rep)) {
						throw new (e) SyntaxError(rep.pos, S("Expected ?, *, +, ->, @ or identifier."));
					} else {
						result->repCapture = parseCapture(e, tok, rep);
					}
				} else if (tok.skipIf(S("["))) {
					// Start of an indented block.
					result->indentStart = result->tokens->count();
				} else if (tok.skipIf(S("]"))) {
					// End of an indented block. See what kind of indentation to use.
					result->indentEnd = result->tokens->count();

					Token kind = tok.next();
					if (kind == S("+")) {
						result->indentType = indentIncrease;
					} else if (kind == S("-")) {
						result->indentType = indentDecrease;
					} else if (kind == S("?")) {
						result->indentType = indentWeakIncrease;
					} else if (kind == S("@")) {
						result->indentType = indentAlignBegin;
					} else if (kind == S("$")) {
						result->indentType = indentAlignEnd;
					} else {
						throw new (e) SyntaxError(kind.pos, TO_S(e, S("Unexpected indentation kind: ") << kind.toS()));
					}
				} else {
					TokenDecl *token = parseToken(e, tok);
					if (token)
						result->tokens->push(token);
				}
			}

			if (tok.skipIf(S("=")))
				result->name = tok.next().toS();

			return result;
		}

		// Parse a production with a result.
		static ProductionDecl *parseProductionResult(Engine &e, Tokenizer &tok, SrcName *rule) {
			tok.expect(S("=>"));

			Name *name = parseName(e, tok);
			Array<Str *> *params = parseActuals(e, tok);

			ProductionDecl *result = parseProduction(e, tok, rule);
			result->result = name;
			result->resultParams = params;

			return result;
		}

		// Parse a production with a priority.
		static ProductionDecl *parseProductionPriority(Engine &e, Tokenizer &tok, SrcName *rule) {
			tok.expect(S("["));
			Int prio = 0;
			try {
				if (tok.skipIf(S("-"))) {
					prio = -tok.next().toS()->toInt();
				} else if (tok.skipIf(S("+"))) {
					prio = tok.next().toS()->toInt();
				} else {
					prio = tok.next().toS()->toInt();
				}
			} catch (const StrError *e) {
				const NException *z = e;
				throw new (e) SyntaxError(tok.position(), z->message());
			}
			tok.expect(S("]"));

			ProductionDecl *result = null;
			Token sep = tok.peek();
			if (sep == S(":")) {
				result = parseProduction(e, tok, rule);
			} else if (sep == S("=>")) {
				result = parseProductionResult(e, tok, rule);
			} else {
				throw new (e) SyntaxError(sep.pos, TO_S(e, S("Unexpected token: ") << sep.toS()));
			}

			result->priority = prio;
			return result;
		}

		// Parse a rule declaration.
		static RuleDecl *parseRule(Engine &e, Tokenizer &tok, SrcName *result) {
			Token name = tok.next();
			RuleDecl *r = new (e) RuleDecl(name.pos, name.toS(), result);

			tok.expect(S("("));

			while (tok.peek() != S(")")) {
				tok.skipIf(S(","));
				Name *type = parseName(e, tok);
				Str *name = tok.next().toS();
				r->params->push(ParamDecl(type, name));
			}

			tok.expect(S(")"));

			if (tok.skipIf(S("#")))
				r->color = parseTokenColor(e, tok);

			return r;
		}

		static FileContents *parseFile(Engine &e, Tokenizer &tok) {
			FileContents *r = new (e) FileContents();

			while (tok.more()) {
				if (tok.skipIf(S("use"))) {
					r->use->push(parseName(e, tok));
				} else if (tok.skipIf(S("delimiter"))) {
					tok.expect(S("="));
					r->delimiter = parseName(e, tok);
				} else {
					SrcName *name = parseName(e, tok);
					Token sep = tok.peek();

					// Comment containind documentation. May be empty, but that's fine.
					SrcPos comment = tok.comment();

					if (sep == S(":")) {
						r->productions->push(applyDoc(comment, parseProduction(e, tok, name)));
					} else if (sep == S("=>")) {
						r->productions->push(applyDoc(comment, parseProductionResult(e, tok, name)));
					} else if (sep == S("[")) {
						r->productions->push(applyDoc(comment, parseProductionPriority(e, tok, name)));
					} else {
						r->rules->push(applyDoc(comment, parseRule(e, tok, name)));
					}
				}

				tok.clearComment();
				tok.expect(S(";"));
			}

			return r;
		}

		FileContents *parseSyntax(Str *data, Url *url, Str::Iter start) {
			Tokenizer tok(url, data, start.offset());
			return parseFile(data->engine(), tok);
		}

	}
}
