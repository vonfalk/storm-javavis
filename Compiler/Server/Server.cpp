#include "stdafx.h"
#include "Server.h"
#include "Engine.h"
#include "Core/Timing.h"

namespace storm {
	namespace server {

		Server::Server(Connection *c) : conn(c) {
			lock = new (this) Lock();
			files = new (this) Map<Nat, File *>();
			colorSyms = new (this) Array<Symbol *>();
			quit = c->symbol(L"quit");
			supported = c->symbol(L"supported");
			open = c->symbol(L"open");
			edit = c->symbol(L"edit");
			point = c->symbol(L"point");
			indent = c->symbol(L"indent");
			close = c->symbol(L"close");
			error = c->symbol(L"error");
			chunkSz = c->symbol(L"chunk-size");
			test = c->symbol(L"test");
			debug = c->symbol(L"debug");
			color = c->symbol(L"color");
			level = c->symbol(L"level");
			as = c->symbol(L"as");
			t = c->symbol(L"t");
			work = new (this) WorkQueue(this);
			chunkChars = defaultChunkChars;
		}

		void Server::run() {
			work->start();
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
			work->stop();
		}

		void Server::runWork(WorkItem *item) {
			try {
				File *f = item->file;
				if (files->get(f->id, null) != f)
					// This is some remaining work from a closed file...
					return;

				Lock::L z(lock);
				Range r = item->run(work);
				updateLater(f, r);
			} catch (const Exception &e) {
				print(TO_S(this, L"While doing background work:"));
				print(::toS(e));
			}
		}

		Bool Server::process(SExpr *msg) {
			Lock::L z(lock);

			Cons *cell = msg->asCons();
			Symbol *kind = cell->first->asSym();

			if (quit->equals(kind)) {
				return false;
			} else if (supported->equals(kind)) {
				work->poke();
				onSupported(cell->rest);
			} else if (open->equals(kind)) {
				work->poke();
				onOpen(cell->rest);
			} else if (edit->equals(kind)) {
				work->poke();
				onEdit(cell->rest);
			} else if (point->equals(kind)) {
				onPoint(cell->rest);
			} else if (close->equals(kind)) {
				work->poke();
				onClose(cell->rest);
			} else if (error->equals(kind)) {
				work->poke();
				onError(cell->rest);
			} else if (indent->equals(kind)) {
				work->poke();
				onIndent(cell->rest);
			} else if (chunkSz->equals(kind)) {
				work->poke();
				onChunkSz(cell->rest);
			} else if (test->equals(kind)) {
				work->poke();
				onTest(cell->rest);
			} else if (debug->equals(kind)) {
				work->poke();
				onDebug(cell->rest);
			} else if (color->equals(kind)) {
				work->poke();
				onColor(cell->rest);
			} else {
				print(TO_S(this, L"Unknown message: " << msg));
			}

			return true;
		}

		void Server::onSupported(SExpr *expr) {
			String *ext = next(expr)->asStr();

			SimpleName *n = readerName(ext->v);
			bool ok = engine().scope().find(n) != null;

			conn->send(list(engine(), 3, supported, ext, ok ? t : null));
		}

		void Server::onOpen(SExpr *expr) {
			Nat id = next(expr)->asNum()->v;
			Str *path = next(expr)->asStr()->v;
			Str *content = next(expr)->asStr()->v;
			Nat point = 0;
			if (Cons *last = ::as<Cons>(expr))
				if (last->first)
					point = last->first->asNum()->v;

			Moment start;

			Url *url = parsePath(path);
			File *f = new (this) File(id, url, content, work);
			f->editPos = point;
			files->put(id, f);

			print(TO_S(this, L"Opened " << url->name() << L" in " << (Moment() - start)));

			// Give the initial data on the file.
			updateLater(f, f->full());
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

			f->editId = editId;
			f->editPos = from;
			Range range = f->replace(Range(from, to), replace);

			// Update the range.
			updateLater(f, range);
		}

		void Server::onPoint(SExpr *expr) {
			Nat fileId = next(expr)->asNum()->v;
			Nat pos = next(expr)->asNum()->v;

			File *f = files->get(fileId, null);
			if (!f)
				return;

			f->editPos = pos;
		}

		void Server::onClose(SExpr *expr) {
			Nat id = next(expr)->asNum()->v;

			files->remove(id);
		}

		void Server::onError(SExpr *expr) {
			Nat id = next(expr)->asNum()->v;

			File *f = files->get(id, null);
			if (!f)
				return;

			try {
				f->findError();
				print(L"No errors.\n");
			} catch (const SyntaxError &error) {
				print(::toS(error));
			}
		}

		void Server::onIndent(SExpr *expr) {
			Nat file = next(expr)->asNum()->v;
			Nat pos = next(expr)->asNum()->v;

			File *f = files->get(file, null);
			if (!f)
				return;

			syntax::TextIndent r = f->indent(pos);
			Engine &e = engine();
			SExpr *result = null;
			if (r.isAlign()) {
				result = list(e, 2, as, new (e) Number(r.alignAs()));
			} else {
				result = list(e, 2, level, new (e) Number(max(0, r.level())));
			}

			result = cons(e, new (e) Number(file), result);
			result = cons(e, indent, result);
			conn->send(result);
		}

		void Server::onChunkSz(SExpr *expr) {
			if (expr == null) {
				SExpr *r = null;
				r = cons(engine(), new (this) Number(chunkChars), r);
				r = cons(engine(), chunkSz, r);
				conn->send(r);
			} else {
				chunkChars = next(expr)->asNum()->v;
				if (chunkChars == 0)
					work->idleTime = 0;
				else
					work->idleTime = WorkQueue::defaultIdleTime;
			}
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

		void Server::onColor(SExpr *expr) {
			Nat fileId = next(expr)->asNum()->v;
			File *f = files->get(fileId, null);
			if (!f) {
				print(TO_S(this, L"No file with id " << fileId));
				return;
			}

			if (chunkChars == 0) {
				update(f, f->full());
			} else {
				updateLater(f, f->full());
			}
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

		void Server::update(File *file, Range range) {
			if (range.empty() && chunkChars != 0)
				// In case 'chunkChars == 0', we always want to send a message.
				return;

			Array<ColoredRange> *colors = file->colors(range);
			Nat pos = range.from;
			if (!colors->empty())
				pos = min(pos, colors->at(0).range.from);
			Array<SExpr *> *result = new (this) Array<SExpr *>();
			// A bit generous, but this array will not live long anyway.
			result->reserve(colors->count() * 4 + 8);

			result->push(color);
			result->push(new (this) Number(file->id));
			result->push(new (this) Number(file->editId));
			result->push(new (this) Number(pos));

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

			// Append a nil entry to the end?
			if (pos < range.to) {
				result->push(new (this) Number(range.to - pos));
				result->push(null);
			}

			conn->send(list(result));
		}

		void Server::updateLater(File *file, Range range) {
			// Shall we send anything at all? 'chunkChars == 0' means that updates have to be queried by the client.
			if (chunkChars == 0)
				return;

			// Do we actually need to split this chunk at all?
			if (range.count() <= chunkChars) {
				update(file, range);
				return;
			}

			// Figure out the first piece to send. We want to pick a piece as close to the last
			// known cursor as possible.
			Nat point = file->editPos;
			Nat halfChunk = chunkChars / 2;
			Range send = range;
			Range todo1, todo2;
			if (point <= range.from + halfChunk) {
				// Send the beginning of 'range'.
				send.to = send.from + chunkChars;
				todo1 = Range(send.to, range.to);
			} else if (point + halfChunk >= range.to) {
				// Send the end of 'range'.
				send.from = send.to - chunkChars;
				todo1 = Range(range.from, send.from);
			} else {
				// Send the range centered around 'point'.
				send.from = point - halfChunk;
				send.to = point + halfChunk;
				todo1 = Range(range.from, send.from);
				todo2 = Range(send.to, range.to);
			}

			// Send the current chunk and schedule the remaining.
			update(file, send);

			// Schedule.
			if (todo1.empty() && todo2.empty())
				return;

			UpdateFileRange *u = new (this) UpdateFileRange(file);
			u->add(todo1);
			u->add(todo2);
			work->post(u);
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


		/**
		 * Scheduled updates.
		 */

		UpdateFileRange::UpdateFileRange(File *file) : WorkItem(file) {
			update = new (this) RangeSet();
		}

		UpdateFileRange::UpdateFileRange(File *file, Range range) : WorkItem(file) {
			update = new (this) RangeSet(range);
		}

		void UpdateFileRange::add(Range range) {
			update->insert(range);
		}

		Bool UpdateFileRange::merge(WorkItem *other) {
			if (!WorkItem::merge(other))
				return false;

			UpdateFileRange *o = (UpdateFileRange *)other;
			update->insert(o->update);
			return true;
		}

		Range UpdateFileRange::run(WorkQueue *q) {
			if (update->empty())
				return Range();

			Range r = update->nearest(file->editPos);
			update->remove(r);

			// Post the modified version of ourselves.
			if (update->any())
				q->post(this);

			return r;
		}

	}
}
