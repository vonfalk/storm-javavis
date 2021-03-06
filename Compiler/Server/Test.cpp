#include "stdafx.h"
#include "Test.h"

namespace storm {
	namespace server {

		Test::Test(Connection *c) : conn(c) {
			start = c->symbol(S("start"));
			stop = c->symbol(S("stop"));
			sum = c->symbol(S("sum"));
			send = c->symbol(S("send"));
			echo = c->symbol(S("echo"));
		}

		void Test::clear() {
			sumResult = 0;
		}

		SExpr *Test::onMessage(SExpr *msg) {
			Cons *cell = msg->asCons();
			Symbol *kind = cell->first->asSym();

			if (!kind) {
				return null;
			} else if (*start == *kind) {
				clear();
			} else if (*stop == *kind) {
				return onStop(cell->rest);
			} else if (*sum == *kind) {
				return onSum(cell->rest);
			} else if (*send == *kind) {
				return onSend(cell->rest);
			} else if (*echo == *kind) {
				return onEcho(cell->rest);
			}

			return null;
		}

		SExpr *Test::onStop(SExpr *msg) {
			if (sumResult > 0)
				return list(engine(), 2, sum, new (this) Number(sumResult));

			return null;
		}

		SExpr *Test::onSum(SExpr *msg) {
			while (msg) {
				SExpr *at = next(msg);
				if (Number *n = as<Number>(at)) {
					sumResult += n->v;
				} else if (String *s = as<String>(at)) {
					for (Str::Iter i = s->v->begin(), end = s->v->end(); i != end; ++i) {
						sumResult += i.v().codepoint();
					}
				}
			}

			return null;
		}

		SExpr *Test::onSend(SExpr *msg) {
			Nat times = next(msg)->asNum()->v;
			SExpr *send = next(msg);
			SExpr *pad = next(msg);

			for (nat i = 0; i < times; i++) {
				conn->send(send);
				if (String *s = as<String>(pad)) {
					conn->textOut->write(s->v);
					conn->textOut->flush();
				}
			}

			return list(engine(), 1, stop);
		}

		SExpr *Test::onEcho(SExpr *msg) {
			return cons(engine(), echo, null);
		}

	}
}
