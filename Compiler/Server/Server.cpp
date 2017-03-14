#include "stdafx.h"
#include "Server.h"
#include "Core/Timing.h"

namespace storm {
	namespace server {

		Server::Server(Connection *c) : conn(c) {
			files = new (this) Map<Nat, File *>();
			colorSyms = new (this) Array<Symbol *>();
			quit = c->symbol(L"quit");
			open = c->symbol(L"open");
			edit = c->symbol(L"edit");
			point = c->symbol(L"point");
			indent = c->symbol(L"indent");
			close = c->symbol(L"close");
			test = c->symbol(L"test");
			debug = c->symbol(L"debug");
			recolor = c->symbol(L"recolor");
			color = c->symbol(L"color");
			level = c->symbol(L"level");
			as = c->symbol(L"as");
			work = new (this) WorkQueue(this);
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
				Range r = item->run(work);
				updateLater(f, r);
			} catch (const Exception &e) {
				print(TO_S(this, L"While doing background work:"));
				print(::toS(e));
			}
		}

		Bool Server::process(SExpr *msg) {
			Cons *cell = msg->asCons();
			Symbol *kind = cell->first->asSym();

			if (quit->equals(kind)) {
				return false;
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
			} else if (indent->equals(kind)) {
				work->poke();
				onIndent(cell->rest);
			} else if (test->equals(kind)) {
				work->poke();
				onTest(cell->rest);
			} else if (debug->equals(kind)) {
				work->poke();
				onDebug(cell->rest);
			} else if (recolor->equals(kind)) {
				work->poke();
				onReColor(cell->rest);
			} else {
				print(TO_S(this, L"Unknown message: " << msg));
			}

			return true;
		}

		void Server::onOpen(SExpr *expr) {
			Nat id = next(expr)->asNum()->v;
			Str *path = next(expr)->asStr()->v;
			Str *content = next(expr)->asStr()->v;

			Moment start;

			File *f = new (this) File(id, parsePath(path), content, work);
			files->put(id, f);

			print(TO_S(this, L"Opened " << path << L" in " << (Moment() - start)));

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

		void Server::onReColor(SExpr *expr) {
			Nat fileId = next(expr)->asNum()->v;
			File *f = files->get(fileId, null);
			if (!f) {
				print(TO_S(this, L"No file with id " << fileId));
				return;
			}

			updateLater(f, f->full());
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
			if (range.empty())
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
			parts = new (this) Array<Range>();
		}

		UpdateFileRange::UpdateFileRange(File *file, Range range) : WorkItem(file) {
			parts = new (this) Array<Range>();
			if (!range.empty())
				parts->push(range);
		}

		void UpdateFileRange::add(Range range) {
			if (range.empty())
				return;

			Nat insertAfter = 0;
			while (insertAfter < parts->count()) {
				Range at = parts->at(insertAfter);

				if (at.intersects(range)) {
					// Remove the one in the array and try to insert the new, expanded range.
					range = storm::server::merge(at, range);
					parts->remove(insertAfter);
				} else if (range.to < at.from) {
					// No need for further examination, we found the right place!
					break;
				} else {
					// Keep looking for the correct place.
					insertAfter++;
				}
			}

			if (insertAfter >= parts->count())
				parts->push(range);
			else
				parts->insert(insertAfter + 1, range);
		}

		Bool UpdateFileRange::merge(WorkItem *other) {
			if (!WorkItem::merge(other))
				return false;

			UpdateFileRange *o = (UpdateFileRange *)other;

			for (Nat i = 0; i < o->parts->count(); i++)
				add(o->parts->at(i));

			return true;
		}

		static Nat delta(Nat a, Nat b) {
			if (a > b)
				return a - b;
			else
				return b - a;
		}

		static Nat delta(Range range, Nat pos) {
			if (range.contains(pos))
				return 0;
			return min(delta(range.from, pos), delta(range.to, pos));
		}

		Range UpdateFileRange::run(WorkQueue *q) {
			if (parts->empty())
				return Range();

			Range r = findNearest();

			// Post the modified version of ourselves.
			if (parts->any())
				q->post(this);

			return r;
		}

		Range UpdateFileRange::findNearest() {
			Nat point = file->editPos;
			Nat bestId = 0;
			Nat bestDelta = delta(parts->at(0), point);

			for (Nat i = 0; i < parts->count(); i++) {
				Nat d = delta(parts->at(i), point);
				if (d < bestDelta) {
					bestDelta = d;
					bestId = i;
				}
			}

			// Now, we have chosen our target.
			Range r = parts->at(bestId);
			parts->remove(bestId);
			return r;
		}

	}
}
