#include "stdafx.h"
#include "BnfReader.h"
#include "Exception.h"

#include "Utils/FileStream.h"
#include "Utils/TextFile.h"
#include "Tokenizer.h"
#include "Package.h"

namespace storm {

	/**
	 * Interface to the package loader.
	 */

	bnf::Reader::Reader(Auto<PkgFiles> files, Auto<Package> pkg) : PkgReader(files, pkg) {}

	void bnf::Reader::readSyntax(SyntaxRules &to) {
		Auto<ScopeExtra> scope = CREATE(ScopeExtra, engine(), owner);
		scope->extra.push_back(scope->find(Name(L"core.lang")));

		const vector<Path> &f = pkgFiles->files;
		for (nat i = 0; i < f.size(); i++) {
			parseBnf(to, f[i], scope);
		}
	}


	/**
	 * The parsing code.
	 */


	void parseBnf(SyntaxRules &types, Tokenizer &tok, Auto<Scope> scope);


	/**
	 * This may change later.
	 */
	bool isBnfFile(const Path &file) {
		return file.hasExt(L"bnf");
	}

	void parseBnf(SyntaxRules &types, const Path &file, Auto<Scope> scope) {
		String content = readTextFile(file);

		Tokenizer tok(file, content, 0);
		parseBnf(types, tok, scope);
	}

	SyntaxRule &getRule(SyntaxRules &map, const String &name) {
		SyntaxRule *t = map[name];
		if (t)
			return *t;

		t = new SyntaxRule(name);
		map.add(t);
		return *t;
	}

	/**
	 * Parsing the bnf.
	 */

	vector<String> parseCallParams(Tokenizer &tok) {
		vector<String> r;
		if (tok.peek().token == L")") {
			tok.next();
			return r;
		}

		bool end = false;
		while (!end) {
			Token param = tok.next();
			r.push_back(param.token);

			Token sep = tok.next();
			if (sep.token == L")") {
				end = true;
			} else if (sep.token == L",") {
			} else {
				throw SyntaxError(sep.pos, L"Unexpected " + sep.token);
			}
		}

		return r;
	}

	void parseCall(SyntaxOption &to, Tokenizer &tok) {
		Token name = tok.next();
		vector<String> params;
		bool isCall = false;

		if (tok.peek().token != L":") {
			Token paren = tok.next();

			if (paren.token == L"(") {
				params = parseCallParams(tok);
				isCall = true;
			}
		}

		Token endSt = tok.next();
		if (endSt.token != L":")
			throw SyntaxError(endSt.pos, L"Expected :");

		to.matchFn = name.token;
		to.matchFnParams = params;
		to.matchVar = !isCall;

		if (!isCall && to.matchFn.size() > 1) {
			throw SyntaxError(name.pos, L"Looks like a function call but has no parameter list!");
		}
	}

	bool isEndOfToken(const Token &t) {
		return (t.token == L",")
			|| (t.token == L"-")
			|| (t.token == L";")
			|| (t.token == L")")
			|| (t.token == L"(");
	}

	SyntaxToken *parseToken(Tokenizer &tok) {
		if (isEndOfToken(tok.peek()))
			return null;

		Token t = tok.next();
		SyntaxToken *result = null;
		vector<String> params;

		if (tok.peek().token == L"(") {
			tok.next();
			params = parseCallParams(tok);
		}

		if (isEndOfToken(tok.peek())) {
			if (t.isStr())
				result = new RegexToken(t.strVal().unescape(true));
			else
				result = new TypeToken(t.token);
		} else {
			Token name = tok.next();
			bool method = false;

			if (name.isStr())
				throw SyntaxError(name.pos, L"Expected name or separator, found string.");

			if (name.token == L"->") {
				method = true;
				name = tok.next();
			}

			if (t.isStr())
				result = new RegexToken(t.strVal().unescape(true), name.token, method);
			else
				result = new TypeToken(t.token, name.token, method);
		}

		TypeToken *tt = as<TypeToken>(result);
		if (tt) {
			tt->params = params;
		} else if (params.size() > 0) {
			delete result;
			throw SyntaxError(t.pos, L"Regex can not take parameters.");
		}

		return result;
	}

	void parseTokens(SyntaxOption &to, Tokenizer &tok) {
		bool end = false;
		while (!end) {
			SyntaxToken *st = parseToken(tok);
			if (st)
				to.add(st);

			Token sep = tok.next();
			if (sep.token == L";") {
				end = true;
			} else if (sep.token == L",") {
				to.add(new DelimToken());
			} else if (sep.token == L"-") {
			} else if (sep.token == L"(") {
				if (to.hasMark())
					throw SyntaxError(sep.pos, L"Only one mark per rule is allowed.");
				to.startMark();
			} else if (sep.token == L")") {
				Token r = tok.next();
				if (r.token == L"*") {
					to.endMark(SyntaxOption::mRepZeroPlus);
				} else if (r.token == L"+") {
					to.endMark(SyntaxOption::mRepOnePlus);
				} else if (r.token == L"?") {
					to.endMark(SyntaxOption::mRepZeroOne);
				} else if (r.token != L"," && r.token != L"-") {
					to.endMark(r.token);
				} else {
					throw SyntaxError(r.pos, L"Unknown repetition: " + r.token);
				}
			} else {
				throw SyntaxError(sep.pos, L"Unknown separator: " + sep.token);
			}
		}
	}

	void parseRule(SyntaxRule &to, Tokenizer &tok, Auto<Scope> scope) {
		SyntaxOption *option = new SyntaxOption(tok.position(), scope, to.name());

		try {
			parseCall(*option, tok);
			parseTokens(*option, tok);

			to.add(option);
		} catch (...) {
			delete option;
			throw;
		}
	}

	void parseDeclaration(SyntaxRule &to, Tokenizer &tok) {
		if (!to.declared.unknown())
			throw SyntaxError(tok.position(), L"There is already a declaration of " + to.name());
		to.declared = tok.position();

		bool end = false;
		if (tok.peek().token == L")")
			end = true;

		while (!end) {
			Token type = tok.next();
			Token name = tok.next();

			Token delim = tok.next();
			if (delim.token == L")")
				end = true;
			else if (delim.token != L",")
				throw SyntaxError(delim.pos, L"Expected , or )");

			SyntaxRule::Param p = { type.token, name.token };
			to.params.push_back(p);
		}

		Token paren = tok.next();
		if (paren.token != L";")
			throw SyntaxError(paren.pos, L"Expected ;");
	}

	void parseBnf(SyntaxRules &types, Tokenizer &tok, Auto<Scope> scope) {
		while (tok.more()) {
			// What we have here is either a rule or a rule declaration.
			Token ruleName = tok.next();
			Token delim = tok.next();

			if (delim.token == L"=>") {
				parseRule(getRule(types, ruleName.token), tok, scope);
			} else if (delim.token == L"(") {
				parseDeclaration(getRule(types, ruleName.token), tok);
			} else {
				throw SyntaxError(delim.pos, L"No rule definition or declaration.");
			}
		}
	}

}
