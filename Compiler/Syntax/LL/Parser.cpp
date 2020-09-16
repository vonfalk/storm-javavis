#include "stdafx.h"
#include "Parser.h"

namespace storm {
	namespace syntax {
		namespace ll {

			Parser::Parser() {}

			void Parser::add(Rule *rule) {}

			void Parser::add(ProductionType *prod) {}

			Bool Parser::sameSyntax(ParserBackend *o) {
				return false;
			}

			Bool Parser::parse(Rule *root, Str *str, Url *file, Str::Iter start) {
				return false;
			}

			InfoErrors Parser::parseApprox(Rule *root, Str *str, Url *file, Str::Iter start, MAYBE(InfoInternal *) ctx) {
				return InfoErrors();
			}

			void Parser::clear() {}

			Bool Parser::hasError() const {
				return false;
			}

			Bool Parser::hasTree() const {
				return false;
			}

			Str::Iter Parser::matchEnd() const {
				return Str::Iter();
			}

			Str *Parser::errorMsg() const {
				return null;
			}

			SrcPos Parser::errorPos() const {
				return SrcPos();
			}

			Node *Parser::tree() const {
				return null;
			}

			InfoNode *Parser::infoTree() const {
				return null;
			}

			Nat Parser::stateCount() const {
				return 0;
			}

			Nat Parser::byteCount() const {
				return 0;
			}

		}
	}
}
