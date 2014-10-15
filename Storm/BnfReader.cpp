#include "stdafx.h"
#include "BnfReader.h"
#include "Exception.h"

#include "Utils/FileStream.h"
#include "Utils/TextFile.h"
#include "Tokenizer.h"

namespace storm {

	typedef hash_map<String, SyntaxRule*> TypeMap;

	void parseBnf(TypeMap &types, Tokenizer &tok);


	/**
	 * This may change later, one idea is to have the extension as:
	 * foo.bnf.<lang> instead of just .bnf
	 */
	bool isBnfFile(const Path &file) {
		return file.hasExt(L"bnf");
	}

	void parseBnf(hash_map<String, SyntaxRule*> &types, const Path &file) {
		util::TextReader *r = util::TextReader::create(new util::FileStream(file, util::Stream::mRead));
		String content = r->getAll();
		delete r;

		Tokenizer tok(file, content, 0);
		parseBnf(types, tok);
	}

	SyntaxRule &getType(TypeMap &map, const String &name) {
		TypeMap::iterator i = map.find(name);
		if (i != map.end())
			return *i->second;

		SyntaxRule *t = new SyntaxRule(name);
		map.insert(make_pair(name, t));
		return *t;
	}

	/**
	 * Parsing the bnf.
	 */

	void parseOutputDirective(SyntaxRule &to, Tokenizer &tok) {
		Token outputStr = tok.next();
		Token end = tok.next();

		if (!outputStr.isStr())
			throw SyntaxError(outputStr.pos, L"Expected string");

		if (end.token != L";")
			throw SyntaxError(end.pos, L"Expected ;");

		to.setOutput(outputStr.strVal().unescape(true));
	}

	bool isEndOfToken(const Token &t) {
		return (t.token == L",")
			|| (t.token == L"-")
			|| (t.token == L"=")
			|| (t.token == L")")
			|| (t.token == L"(");
	}

	SyntaxToken *parseToken(Tokenizer &tok) {
		if (isEndOfToken(tok.peek()))
			return null;

		Token t = tok.next();

		if (isEndOfToken(tok.peek())) {
			if (t.isStr())
				return new RegexToken(t.strVal().unescape(true));
			else
				return new TypeToken(t.token);
		} else {
			Token name = tok.next();
			if (name.isStr())
				throw SyntaxError(name.pos, L"Expected name or separator, found string.");

			if (t.isStr())
				return new RegexToken(t.strVal().unescape(true), name.token);
			else
				return new TypeToken(t.token, name.token);
		}
	}

	void parseTokens(SyntaxOption &to, Tokenizer &tok) {
		bool end = false;
		while (!end) {
			SyntaxToken *st = parseToken(tok);
			if (st)
				to.add(st);

			Token sep = tok.next();
			if (sep.token == L"=") {
				end = true;
			} else if (sep.token == L",") {
				to.add(new DelimToken());
			} else if (sep.token == L"-") {
			} else if (sep.token == L"(") {
				if (to.hasRepeat())
					throw SyntaxError(sep.pos, L"Only one repeat per rule is allowed.");
				to.startRepeat();
			} else if (sep.token == L")") {
				Token r = tok.next();
				if (r.token == L"*") {
					to.endRepeat(SyntaxOption::rZeroPlus);
				} else if (r.token == L"+") {
					to.endRepeat(SyntaxOption::rOnePlus);
				} else if (r.token == L"?") {
					to.endRepeat(SyntaxOption::rZeroOne);
				} else {
					throw SyntaxError(r.pos, L"Unknown repetition: " + r.token);
				}
			} else {
				throw SyntaxError(sep.pos, L"Unknown separator: " + sep.token);
			}
		}
	}

	void parseCall(SyntaxOption &to, Tokenizer &tok) {
		Token name = tok.next();
		Token paren = tok.next();

		if (paren.token != L"(")
			throw SyntaxError(paren.pos, L"Expected (");

		vector<String> params;
		bool end = false;
		while (!end) {
			Token param = tok.next();
			if (param.token == L")")
				break;

			params.push_back(param.token);

			Token sep = tok.next();
			if (sep.token == L")") {
				end = true;
			} else if (sep.token == L",") {
			} else {
				throw SyntaxError(sep.pos, L"Unexpected " + sep.token);
			}
		}

		Token endSt = tok.next();
		if (endSt.token != L";")
			throw SyntaxError(endSt.pos, L"Expected ;");

		to.setMatchFn(name.token, params);
	}

	void parseRule(SyntaxRule &to, Tokenizer &tok) {
		SyntaxOption *rule = new SyntaxOption(tok.position());

		try {
			parseTokens(*rule, tok);
			parseCall(*rule, tok);

			to.add(rule);
		} catch (...) {
			delete rule;
			throw;
		}
	}

	void parseBnf(TypeMap &types, Tokenizer &tok) {
		while (tok.more()) {
			// What we have here is either a rule or a output directive:
			Token typeName = tok.next();
			Token delim = tok.next();

			if (delim.token == L"=>") {
				parseOutputDirective(getType(types, typeName.token), tok);
			} else if (delim.token == L":") {
				parseRule(getType(types, typeName.token), tok);
			} else {
				throw SyntaxError(delim.pos, L"Unknown rule or directive");
			}
		}
	}

}
