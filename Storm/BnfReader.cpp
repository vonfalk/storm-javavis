#include "stdafx.h"
#include "BnfReader.h"

#include "Utils/FileStream.h"
#include "Utils/TextFile.h"
#include "Tokenizer.h"

namespace storm {

	void parseBnf(hash_map<String, SyntaxType*> &types, Tokenizer &tok);


	/**
	 * This may change later, one idea is to have the extension as:
	 * foo.bnf.<lang> instead of just .bnf
	 */
	bool isBnfFile(const Path &file) {
		return file.hasExt(L"bnf");
	}

	void parseBnf(hash_map<String, SyntaxType*> &types, const Path &file) {
		util::TextReader *r = util::TextReader::create(new util::FileStream(file, util::Stream::mRead));
		String content = r->getAll();
		delete r;

		Tokenizer tok(file, content, 0);
		parseBnf(types, tok);
	}

	void parseBnf(hash_map<String, SyntaxType*> &types, Tokenizer &tok) {
		while (tok.more())
			PLN(tok.next().token);
	}

}
