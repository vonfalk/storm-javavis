#include "stdafx.h"
#include "Repl.h"

namespace storm {

	Repl::Result::Result(int type, Str *value) : type(type), data(value) {}

	Repl::Result Repl::Result::success() {
		return Result(rSuccess, null);
	}

	Repl::Result Repl::Result::success(Str *value) {
		return Result(rSuccess, value);
	}

	Repl::Result Repl::Result::error(Str *value) {
		return Result(rError, value);
	}

	Repl::Result Repl::Result::incomplete() {
		return Result(rIncomplete, null);
	}

	Repl::Result Repl::Result::terminate() {
		return Result(rTerminate, null);
	}

	Bool Repl::Result::isSuccess() const {
		return type == rSuccess;
	}

	MAYBE(Str *) Repl::Result::result() const {
		if (type == rSuccess)
			return data;
		return null;
	}

	MAYBE(Str *) Repl::Result::isError() const {
		if (type == rError)
			return data;
		return null;
	}

	Bool Repl::Result::isIncomplete() const {
		return type == rIncomplete;
	}

	Bool Repl::Result::isTerminate() const {
		return type == rTerminate;
	}

	Repl::Repl() {}

	void Repl::greet() {}

}
