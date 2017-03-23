#include "stdafx.h"
#include "ParserBackend.h"
#include "Exception.h"

namespace storm {
	namespace syntax {

		ParseResult::ParseResult() : success(false), skippedChars(0), corrections(0) {}

		ParseResult::ParseResult(Nat skipped, Nat corrections) :
			success(true), skippedChars(skipped), corrections(corrections) {}

		StrBuf &operator <<(StrBuf &to, ParseResult r) {
			if (r.success)
				return to << r.skippedChars << L" chars skipped, " << r.corrections << L" corrections";
			else
				return to << L"Failed";
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

		ParseResult ParserBackend::parseApprox(Rule *root, Str *str, Url *file, Str::Iter start) {
			return ParseResult();
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

		Nat ParserBackend::stateCount() const {
			return 0;
		}

		Nat ParserBackend::byteCount() const {
			return 0;
		}

	}
}
