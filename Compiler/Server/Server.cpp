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
			quit = c->symbol(S("quit"));
			supported = c->symbol(S("supported"));
			open = c->symbol(S("open"));
			edit = c->symbol(S("edit"));
			point = c->symbol(S("point"));
			indent = c->symbol(S("indent"));
			close = c->symbol(S("close"));
			error = c->symbol(S("error"));
			chunkSz = c->symbol(S("chunk-size"));
			test = c->symbol(S("test"));
			debug = c->symbol(S("debug"));
			color = c->symbol(S("color"));
			level = c->symbol(S("level"));
			as = c->symbol(S("as"));
			t = c->symbol(S("t"));
			completeName = c->symbol(S("complete-name"));
			documentation = c->symbol(S("documentation"));
			replAvailable = c->symbol(S("repl-available"));
			replEval = c->symbol(S("repl-eval"));
			work = new (this) WorkQueue(this);
			repls = new (this) Map<Str *, Repl *>();
			chunkChars = defaultChunkChars;
		}

		void Server::run() {
			work->start();
			print(S("Language server started."));

			SExpr *msg = null;
			while (msg = conn->receive()) {
				try {
					if (!process(msg))
						break;
				} catch (const MsgError *e) {
					print(TO_S(this, S("While processing ") << msg << S(":")));
					print(e->toS());
				} catch (const Exception *e) {
					// TODO: Better error ouput for errors containing SrcPos.
					print(TO_S(this, S("While processing ") << msg << S(":")));
					print(e->toS());
				}
			}

			print(S("Terminating. Bye!"));
			work->stop();
		}

		void Server::runWork(WorkItem *item) {
			try {
				File *f = item->file;
				if (files->get(f->id, null) != f)
					// This is some remaining work from a closed file...
					return;

				Lock::Guard z(lock);
				Range r = item->run(work);
				updateLater(f, r);
			} catch (const Exception *e) {
				print(TO_S(this, S("While doing background work:")));
				print(e->toS());
			}
		}

		Bool Server::process(SExpr *msg) {
			Lock::Guard z(lock);

			Cons *cell = msg->asCons();
			Symbol *kind = cell->first->asSym();

			if (!kind) {
				return true;
			} else if (*quit == *kind) {
				return false;
			} else if (*supported == *kind) {
				work->poke();
				onSupported(cell->rest);
			} else if (*open == *kind) {
				work->poke();
				onOpen(cell->rest);
			} else if (*edit == *kind) {
				work->poke();
				onEdit(cell->rest);
			} else if (*point == *kind) {
				onPoint(cell->rest);
			} else if (*close == *kind) {
				work->poke();
				onClose(cell->rest);
			} else if (*error == *kind) {
				work->poke();
				onError(cell->rest);
			} else if (*indent == *kind) {
				work->poke();
				onIndent(cell->rest);
			} else if (*chunkSz == *kind) {
				work->poke();
				onChunkSz(cell->rest);
			} else if (*test == *kind) {
				work->poke();
				onTest(cell->rest);
			} else if (*debug == *kind) {
				work->poke();
				onDebug(cell->rest);
			} else if (*color == *kind) {
				work->poke();
				onColor(cell->rest);
			} else if (*completeName == *kind) {
				onComplete(cell->rest);
			} else if (*documentation == *kind) {
				onDocumentation(cell->rest);
			} else if (*replAvailable == *kind) {
				onReplAvailable(cell->rest);
			} else if (*replEval == *kind) {
				onReplEval(cell->rest);
			} else {
				print(TO_S(this, S("Unknown message: ") << msg));
			}

			return true;
		}


		/**
		 * Handle messages.
		 */

		void Server::onSupported(SExpr *expr) {
			String *ext = next(expr)->asStr();

			// TODO: This is not always how Storm works anymore. See 'codeFileType' in 'Reader.h' for details.
			MAYBE(SimpleName *) n = readerName(ext->v);
			bool ok = n && engine().scope().find(n) != null;

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

			print(TO_S(this, S("Opened ") << url->name() << S(" in ") << (Moment() - start)));

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
				print(S("No errors.\n"));
			} catch (const SyntaxError *error) {
				print(error->toS());
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

				if (expr)
					work->idleTime = next(expr)->asNum()->v;
				else if (chunkChars == 0)
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
				print(TO_S(this, S("No file with id ") << fileId));
				return;
			}

			f->debugOutput(conn->textOut, tree);
		}

		void Server::onColor(SExpr *expr) {
			Nat fileId = next(expr)->asNum()->v;
			File *f = files->get(fileId, null);
			if (!f) {
				print(TO_S(this, S("No file with id ") << fileId));
				return;
			}

			if (chunkChars == 0) {
				update(f, f->full());
			} else {
				updateLater(f, f->full());
			}
		}

		static Name *parseName(Str *str) {
			Name *result = parseComplexName(str);
			while (!result) {
				// Try to remove '(' and see if we can parse!
				Str::Iter last = str->findLast(Char('('));
				if (last == str->end())
					break;

				str = str->substr(str->begin(), last);
				result = parseComplexName(str);
			}

			return result;
		}

		static Str *format(Named *f, Name *candidate) {
			Nat len = max(Nat(1), candidate->count());
			SimpleName *path = f->path();
			if (path->count() > len) {
				path = path->from(path->count() - len);
			}
			return path->toS();
		}

		static void findOptions(NameLookup *in, Name *candidate, Array<Str *> *out) {
			NameSet *found = as<NameSet>(in);
			if (!found)
				return;

			// Make sure everything is properly loaded.
			found->forceLoad();
			for (NameSet::Iter i = found->begin(), e = found->end(); i != e; ++i) {
				Named *f = i.v();
				if (candidate->empty())
					out->push(format(f, candidate));
				else if (f->name->startsWith(candidate->last()->name))
					out->push(format(f, candidate));
			}
		}

		static void findOptions(Engine &e, Scope scope, Name *candidate, Array<Str *> *out) {
			Name *parent = candidate->parent();

			if (parent->empty()) {
				// Look inside 'core', 'root' and 'top'.
				findOptions(scope.top, candidate, out);
				Package *core = e.package(S("core"));
				if (scope.top != core)
					findOptions(core, candidate, out);
				if (scope.top != e.package())
					findOptions(e.package(), candidate, out);
			} else {
				findOptions(scope.find(parent), candidate, out);
			}
		}

		static Scope createScope(Engine &e, SExpr *&ctx) {
			Scope scope = e.scope();

			if (ctx) {
				Url *path = parsePath(next(ctx)->asStr()->v);
				if (Package *pkg = e.package(path))
					scope = Scope(scope, pkg);
			}

			return scope;
		}

		void Server::onComplete(SExpr *expr) {
			Str *str = next(expr)->asStr()->v;
			Scope scope = createScope(engine(), expr);

			Array<Str *> *results = new (this) Array<Str *>();
			Name *name = parseName(str);
			if (name) {
				findOptions(engine(), scope, name, results);
			}

			SExpr *reply = null;
			for (Nat i = results->count(); i > 0; i--)
				reply = cons(engine(), new (this) String(results->at(i - 1)), reply);
			conn->send(cons(engine(), completeName, reply));
		}

		SExpr *Server::formatValue(Str *name, Named *type, Bool ref) {
			SExpr *result = null;
			result = cons(engine(), ref ? t : null, result);
			if (type)
				result = cons(engine(), new (this) String(type->identifier()), result);
			else
				result = cons(engine(), null, result);
			result = cons(engine(), new (this) String(name), result);
			return result;
		}

		SExpr *Server::formatValue(Str *name, Value v) {
			return formatValue(name, v.type, v.ref);
		}

		SExpr *Server::formatNote(const DocNote &note) {
			if (note.showType) {
				return formatValue(note.note, note.named, note.ref);
			} else {
				SExpr *title = new (this) String(note.note);
				return cons(engine(), title, null);
			}
		}

		static Bool compareNamed(Named *a, Named *b) {
			if (*a->name == *b->name)
				return a->params->count() < b->params->count();
			return *a->name < *b->name;
		}

		static SExpr *formatNameList(Array<Named *> *items) {
			Engine &e = items->engine();
			SExpr *result = null;
			items->sort(fnPtr(e, &compareNamed));

			for (Nat i = items->count(); i > 0; i--) {
				Named *src = items->at(i - 1);
				// TODO: We might actually want to *load* documentation to provide parameter names here.
				SExpr *m = cons(e, new (e) String(src->identifier()),
								new (e) String(doc(src)->summary()));
				result = cons(e, m, result);
			}

			return result;
		}

		static SExpr *formatRefs(Named *entity) {
			Engine &e = entity->engine();
			SExpr *result = null;

			if (NameSet *s = as<NameSet>(entity)) {
				s->forceLoad();

				Array<Named *> *found = new (e) Array<Named *>();
				for (NameSet::Iter i = s->begin(), end = s->end(); i != end; ++i)
					found->push(i.v());

				SExpr *node = cons(e, new (e) String(S("Members")), formatNameList(found));
				result = cons(e, node, result);
			}

			if (Type *t = as<Type>(entity)) {
				Array<Named *> *found = new (e) Array<Named *>();
				TypeChain::Iter i = t->chain->children();
				while (Type *t = i.next())
					found->push(t);

				if (found->any()) {
					SExpr *node = cons(e, new (e) String(S("Known subclasses")), formatNameList(found));
					result = cons(e, node, result);
				}
			}

			return result;
		}

		SExpr *Server::formatDoc(Named *entity) {
			Engine &e = engine();
			SExpr *data = null;

			Doc *d = entity->findDoc();

			// References.
			{
				SExpr *refs = formatRefs(entity);
				data = cons(e, refs, data);
			}

			// Source position.
			if (entity->pos.any()) {
				SExpr *cell = cons(e, new (e) String(entity->pos.file->toS()),
								cons(e, new (e) Number(entity->pos.start),
									cons(e, new (e) Number(entity->pos.end),
										null)));
				data = cons(e, cell, data);
			} else {
				data = cons(e, null, data);
			}

			// Body. TODO: Maybe a richer representation that includes references etc.?
			data = cons(e, new (e) String(d->body), data);

			// Visibility.
			if (d->visibility) {
				Str *type = runtime::typeOf(d->visibility)->path()->toS();
				SExpr *cell = cons(e, new (e) String(type), new (e) String(d->visibility->toS()));

				data = cons(e, cell, data);
			} else {
				data = cons(e, null, data);
			}

			// Notes.
			{
				SExpr *notes = null;
				for (Nat i = d->notes->count(); i > 0; i--) {
					DocNote n = d->notes->at(i - 1);
					notes = cons(e, formatNote(n), notes);
				}
				data = cons(e, notes, data);
			}

			// Parameters.
			{
				SExpr *params = null;
				for (Nat i = d->params->count(); i > 0; i--) {
					DocParam p = d->params->at(i - 1);
					params = cons(e, formatValue(p.name, p.type), params);
				}
				data = cons(e, params, data);
			}

			// Name.
			data = cons(e, new (e) String(d->name), data);

			return data;
		}

		void Server::findDoc(Name *name, Scope scope, Array<SExpr *> *to) {
			if (!name)
				return;

			if (Named *entity = scope.find(name)) {
				// We found something! Output whatever we know about this entity.
				to->push(formatDoc(entity));
				return;
			}

			if (NameSet *parent = ::as<NameSet>(scope.find(name->parent()))) {
				Array<Named *> *found = parent->findName(name->last()->name);
				found->sort(fnPtr(found->engine(), &compareNamed));

				for (Nat i = 0; i < found->count(); i++) {
					to->push(formatDoc(found->at(i)));
				}
			}
		}

		void Server::onDocumentation(SExpr *expr) {
			Name *name = parseComplexName(next(expr)->asStr()->v);
			Scope scope = createScope(engine(), expr);

			Array<SExpr *> *result = new (this) Array<SExpr *>();
			findDoc(name, scope, result);

			conn->send(list(engine(), 2, documentation, list(result)));
		}

		static bool hasRepl(Package *pkg) {
			SimplePart *part = new (pkg) SimplePart(S("repl"));
			Function *fn = as<Function>(pkg->find(part, Scope()));
			if (!fn)
				return false;

			// Check the return value.
			return Value(StormInfo<Repl>::type(pkg->engine())).canStore(fn->result);
		}

		void Server::onReplAvailable(SExpr *expr) {
			SExpr *result = null;
			bool hasBs = false;

			Package *lang = engine().package(S("lang"));
			for (NameSet::Iter i = lang->begin(), e = lang->end(); i != e; ++i) {
				if (Package *p = ::as<Package>(i.v())) {
					if (hasRepl(p)) {
						if (*p->name == S("bs"))
							hasBs = true;
						else
							result = cons(engine(), new (this) String(p->name), result);
					}
				}
			}

			if (hasBs)
				result = cons(engine(), new (this) String(S("bs")), result);

			result = cons(engine(), replAvailable, result);
			conn->send(result);
		}

		void Server::onReplEval(SExpr *expr) {
			String *name = next(expr)->asStr();
			String *eval = next(expr)->asStr();
			String *context = next(expr)->asStr();

			Repl *repl = repls->get(name->v, null);
			if (!repl) {
				SimpleName *replName = new (this) SimpleName();
				replName->add(new (this) Str(S("lang")));
				replName->add(name->v);
				replName->add(new (this) Str(S("repl")));
				Function *fn = ::as<Function>(engine().scope().find(replName));
				if (fn) {
					if (!Value(StormInfo<Repl>::type(engine())).canStore(fn->result))
						fn = null;
				}

				if (!fn) {
					print(TO_S(this, S("Failed to create REPL for language " << name->v << S("."))));
					return;
				}

				typedef Repl *(CODECALL *CreateRepl)();
				CreateRepl create = (CreateRepl)fn->ref().address();
				repl = (*create)();

				repls->put(name->v, repl);
			}

			Package *ctx = null; // TODO!

			Server *me = this;
			os::FnCall<void, 4> call = os::fnCall().add(me).add(repl).add(eval->v).add(ctx);
			os::UThread::spawn(address(&Server::evalThread), true, call);
		}

		void Server::evalThread(Repl *repl, Str *expr, Package *context) {
			SExpr *result = null;

			if (repl->eval(expr)) {
				result = new (this) String(S("TODO"));
			}

			Lock::Guard z(lock);
			conn->send(list(engine(), 2, replEval, result));
		}

		/**
		 * Update logic.
		 */

		static Str *colorName(EnginePtr e, syntax::TokenColor c) {
			using namespace storm::syntax;
			const wchar *name = S("<unknown>");
			switch (c) {
			case tNone:
				name = S("nil");
				break;
			case tComment:
				name = S("comment");
				break;
			case tDelimiter:
				name = S("delimiter");
				break;
			case tString:
				name = S("string");
				break;
			case tConstant:
				name = S("constant");
				break;
			case tKeyword:
				name = S("keyword");
				break;
			case tFnName:
				name = S("fn-name");
				break;
			case tVarName:
				name = S("var-name");
				break;
			case tTypeName:
				name = S("type-name");
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
