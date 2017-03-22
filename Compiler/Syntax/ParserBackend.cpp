#include "stdafx.h"
#include "ParserBackend.h"
#include "Exception.h"

namespace storm {
	namespace syntax {

		ParseResult::ParseResult(InfoNode *tree, Nat skipped, Nat corrections) :
			tree(tree), skippedChars(skipped), corrections(corrections) {}

		StrBuf &operator <<(StrBuf &to, ParseResult r) {
			return to << r.skippedChars << L" chars skipped, " << r.corrections << L" corrections";
		}


		ParserBackend::ParserBackend() {}

		void ParserBackend::add(Rule *r) {}

		void ParserBackend::add(ProductionType *p) {}

		Bool ParserBackend::sameSyntax(ParserBackend *o) {
			return true;
		}

		Bool ParserBackend::parse(Rule *root, Str *str, Url *file, Str::Iter start) {
			return false;
		}

		void ParserBackend::clear() {}

		Bool ParserBackend::hasError() const {
			return false;
		}

		Bool ParserBackend::hasTree() const {
			return false;
		}

		Str::Iter ParserBackend::matchEnd() const {
			return Str::Iter();
		}

		Str *ParserBackend::errorMsg() const {
			return new (this) Str(L"no error");
		}

		SrcPos ParserBackend::errorPos() const {
			return SrcPos();
		}

		Node *ParserBackend::tree() const {
			throw InternalError(L"No tree.");
		}

		InfoNode *ParserBackend::infoTree() const {
			throw InternalError(L"No tree.");
		}

		ParseResult ParserBackend::fullInfoTree() {
			throw InternalError(L"No tree.");
		}

		Nat ParserBackend::stateCount() const {
			return 0;
		}

		Nat ParserBackend::byteCount() const {
			return 0;
		}

	}
}
