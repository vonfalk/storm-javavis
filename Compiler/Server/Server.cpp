#include "stdafx.h"
#include "Server.h"

namespace storm {
	namespace server {

		Server::Server(Connection *c) : conn(c) {
			files = new (this) Map<Nat, File *>();
			colorSyms = new (this) Array<Symbol *>();
			quit = c->symbol(L"quit");
			open = c->symbol(L"open");
			edit = c->symbol(L"edit");
			close = c->symbol(L"close");
			test = c->symbol(L"test");
			debug = c->symbol(L"debug");
			color = c->symbol(L"color");
		}

		void Server::run() {
			print(L"Language server started.");

			SExpr *msg = null;
			while (msg = conn->receive()) {
				try {
					if (!process(msg))
						break;
				} catch (const MsgError &e) {
					print(TO_S(this, L"While processing " << msg << L":"));
					print(::toS(e));
				} catch (const Exception &e) {
					// TODO: Better error ouput for errors containing SrcPos.
					print(TO_S(this, L"While processing " << msg << L":"));
					print(::toS(e));
				}
			}

			print(L"Terminating. Bye!");
		}

		Bool Server::process(SExpr *msg) {
			Cons *cell = msg->asCons();
			Symbol *kind = cell->first->asSym();

			if (quit->equals(kind)) {
				return false;
			} else if (open->equals(kind)) {
				onOpen(cell->rest);
			} else if (edit->equals(kind)) {
				onEdit(cell->rest);
			} else if (close->equals(kind)) {
				onClose(cell->rest);
			} else if (test->equals(kind)) {
				onTest(cell->rest);
			} else if (debug->equals(kind)) {
				onDebug(cell->rest);
			} else {
				print(TO_S(this, L"Unknown message: " << msg));
			}

			return true;
		}

		void Server::onOpen(SExpr *expr) {
			Nat id = next(expr)->asNum()->v;
			Str *path = next(expr)->asStr()->v;
			Str *content = next(expr)->asStr()->v;

			File *f = new (this) File(id, parsePath(path), content);
			files->put(id, f);

			// Give the initial data on the file.
			update(f, f->full(), 0);
		}

		void Server::onEdit(SExpr *expr) {
			Nat fileId = next(expr)->asNum()->v;
			Nat editId = next(expr)->asNum()->v;
			Nat from = next(expr)->asNum()->v;
			Nat to = next(expr)->asNum()->v;
			Str *replace = next(expr)->asStr()->v;

			File *f = files->get(fileId, null);
			if (!f)
				return;

			Range range(from, to);
			range = f->replace(range, replace);

			// Update the range.
			update(f, range, editId);
		}

		void Server::onClose(SExpr *expr) {
			Nat id = next(expr)->asNum()->v;

			files->remove(id);
		}

		void Server::onTest(SExpr *expr) {
			if (!testState)
				testState = new (this) Test(conn);

			SExpr *reply = testState->onMessage(expr);
			if (reply)
				conn->send(new (this) Cons(test, reply));
		}

		void Server::onDebug(SExpr *expr) {
			Nat fileId = next(expr)->asNum()->v;
			Bool tree = next(expr) != null;

			File *f = files->get(fileId, null);
			if (!f) {
				print(TO_S(this, L"No file with id " << fileId));
				return;
			}

			f->debugOutput(conn->textOut, tree);
		}

		static Str *colorName(EnginePtr e, syntax::TokenColor c) {
			using namespace storm::syntax;
			const wchar *name = L"<unknown>";
			switch (c) {
			case tNone:
				name = L"nil";
				break;
			case tComment:
				name = L"comment";
				break;
			case tDelimiter:
				name = L"delimiter";
				break;
			case tString:
				name = L"string";
				break;
			case tConstant:
				name = L"constant";
				break;
			case tKeyword:
				name = L"keyword";
				break;
			case tFnName:
				name = L"fn-name";
				break;
			case tVarName:
				name = L"var-name";
				break;
			case tTypeName:
				name = L"type-name";
				break;
			}

			return new (e.v) Str(name);
		}

		Symbol *Server::colorSym(syntax::TokenColor color) {
			if (color == syntax::tNone)
				return null;

			Nat id = Nat(color);
			while (id >= colorSyms->count())
				colorSyms->push(null);

			Symbol *s = colorSyms->at(id);
			if (!s) {
				s = conn->symbol(colorName(engine(), color));
				colorSyms->at(id) = s;
			}

			return s;
		}

		void Server::update(File *file, Range range, Nat editId) {
			Array<ColoredRange> *colors = file->colors(range);
			Array<SExpr *> *result = new (this) Array<SExpr *>();
			// A bit generous, but this array will not live long anyway.
			result->reserve(colors->count() * 4 + 4);

			result->push(color);
			result->push(new (this) Number(file->id));
			result->push(new (this) Number(editId));
			result->push(new (this) Number(range.from));

			Nat pos = range.from;
			for (Nat i = 0; i < colors->count(); i++) {
				ColoredRange r = colors->at(i);

				if (pos < r.range.from) {
					// Clear this range.
					result->push(new (this) Number(r.range.from - pos));
					result->push(null);
				}

				// Add 'r' to the result.
				result->push(new (this) Number(r.range.to - r.range.from));
				result->push(colorSym(r.color));
				pos = r.range.to;
			}

			conn->send(list(result));
		}

		void Server::print(Str *s) {
			conn->textOut->writeLine(s);
		}

		void Server::print(const wchar *s) {
			print(new (this) Str(s));
		}

		void Server::print(const CString &s) {
			print(new (this) Str(s.c_str()));
		}

	}
}
