#include "stdafx.h"
#include "File.h"
#include "Engine.h"

namespace storm {
	namespace server {
		using namespace storm::syntax;

		static Nat strlen(Str *s) {
			Nat len = 0;
			for (Str::Iter i = s->begin(), e = s->end(); i != e; ++i)
				len++;
			return len;
		}

		Part::Part(File *owner, Nat offset, FileReader *part, MAYBE(FileReader *) next) {
			this->owner = owner;
			start = offset;
			path = part->info->url;

			Str *src = part->info->contents;
			Str::Iter expectedFinish = src->end();
			if (next)
				expectedFinish = next->info->start;
			src = src->substr(part->info->start, expectedFinish);

			content = new (this) InfoLeaf(null, src);
			parser = part->createParser();
			if (InfoNode *n = parse(content, parser->root()))
				content = n;
		}

		Bool Part::replace(FileReader *reader, MAYBE(FileReader *) next) {
			InfoParser *p = reader->createParser();
			if (parser->sameSyntax(p))
				return false;

			Str *src = reader->info->contents;
			Str::Iter expectedFinish = src->end();
			if (next)
				expectedFinish = next->info->start;
			src = src->substr(reader->info->start, expectedFinish);

			InfoNode *c = new (this) InfoLeaf(null, src);
			parser = p;
			if (InfoNode *n = parse(c, parser->root()))
				content = n;

			return true;
		}

		Range Part::full() const {
			return Range(start, start + content->length());
		}

		Range Part::replace(Range range, Str *replace) {
			if (!content)
				return Range(0, 0);

			// Do all regexes match?
			Bool regexMatching = true;

			// Anything to remove?
			if (!range.empty())
				regexMatching &= remove(range);

			// Anything to add?
			if (replace->begin() != replace->end())
				regexMatching &= insert(range.from, replace);

			Range result(range.from, range.from + strlen(replace));
			if (!regexMatching) {
				result = merge(result, parse(range));
			}

			return result;
		}

		Bool Part::remove(const Range &range) {
			return remove(range, start, content);
		}

		Bool Part::remove(const Range &range, Nat offset, InfoNode *node) {
			Nat len = node->length();
			if (len == 0)
				return true;

			Range nodeRange(offset, offset + len);
			if (!nodeRange.intersects(range))
				return true;

			if (InfoLeaf *leaf = as<InfoLeaf>(node)) {
				return removeLeaf(range, offset, leaf);
			} else if (InfoInternal *inode = as<InfoInternal>(node)) {
				return removeInternal(range, offset, inode);
			} else {
				assert(false, L"This should not happen!");
				return false; // Force a re-parse.
			}
		}

		Bool Part::removeLeaf(const Range &range, Nat offset, InfoLeaf *node) {
			Str *src = node->toS();
			Str::Iter begin = src->begin();
			for (Nat i = offset; i < range.from; i++)
				++begin;

			Str::Iter end = begin;
			for (Nat i = max(offset, range.from); i < range.to; i++)
				++end;

			node->set(src->remove(begin, end));

			return node->matchesRegex();
		}

		Bool Part::removeInternal(const Range &range, Nat offset, InfoInternal *node) {
			Bool ok = true;

			for (Nat i = 0; i < node->count(); i++) {
				InfoNode *at = node->at(i);
				// Keep track of the old length, as that will likely change.
				Nat len = at->length();
				ok &= remove(range, offset, at);
				offset += len;
			}

			return ok;
		}

		struct Part::InsertState {
			// Insert 'v' at 'point'.
			Nat point;
			Str *v;

			// Anything inserted so far?
			Bool done;

			// First viable match. Maybe not chosen due to regex mismatch.
			InfoLeaf *firstPossible;
			Str *firstPossibleStr;
		};

		Bool Part::insert(Nat point, Str *v) {
			InsertState state = {
				point,
				v,
				false,
				null,
			};
			insert(state, start, content);

			// If 'done' is true, then we succeeded at inserting the string somewhere where the
			// regex matched.
			if (state.done)
				return true;

			// We should insert it at 'firstPossible'.
			assert(state.firstPossible, L"No node for insertion was found!");
			if (!state.firstPossible)
				return false; // Force a re-parse.

			// We can not avoid invalidating a regex...
			state.firstPossible->set(state.firstPossibleStr);
			return false;
		}

		void Part::insert(InsertState &state, Nat offset, InfoNode *node) {
			// Outside this node?
			Nat len = node->length();
			if (state.point < offset || state.point > offset + len)
				return;

			if (InfoLeaf *leaf = as<InfoLeaf>(node)) {
				insertLeaf(state, offset, leaf);
			} else if (InfoInternal *inode = as<InfoInternal>(node)) {
				insertInternal(state, offset, inode);
			} else {
				assert(false, L"This should not happen!");
			}
		}

		void Part::insertLeaf(InsertState &state, Nat offset, InfoLeaf *node) {
			Str *src = node->toS();
			Str::Iter p = src->begin();
			for (Nat i = offset; i < state.point; i++)
				++p;

			Str *modified = src->insert(p, state.v);
			if (!state.firstPossible) {
				state.firstPossible = node;
				state.firstPossibleStr = modified;
			}

			if (node->matchesRegex(modified)) {
				node->set(modified);
				state.done = true;
			}
		}

		void Part::insertInternal(InsertState &state, Nat offset, InfoInternal *node) {
			for (Nat i = 0; i < node->count() && !state.done; i++) {
				InfoNode *at = node->at(i);
				Nat len = at->length();
				insert(state, offset, at);
				offset += len;
			}
		}

		Array<ColoredRange> *Part::colors(Range range) {
			Array<ColoredRange> *r = new (this) Array<ColoredRange>();
			if (content)
				colors(r, range, start, content);
			return r;
		}

		void Part::colors(Array<ColoredRange> *out, const Range &range, Nat offset, InfoNode *node) {
			// We can ignore zero-length nodes.
			Nat len = node->length();
			if (len == 0)
				return;

			Range nodeRange(offset, offset + len);
			if (!nodeRange.intersects(range))
				return;

			// Pick the root-most color.
			if (node->color != syntax::tNone) {
				out->push(ColoredRange(nodeRange, node->color));
				return;
			}

			// Else, continue traversing.
			if (InfoInternal *n = as<InfoInternal>(node)) {
				Nat nodeOffset = offset;
				for (Nat i = 0; i < n->count(); i++) {
					colors(out, range, nodeOffset, n->at(i));
					nodeOffset += n->at(i)->length();
				}
			}
		}

		InfoNode *Part::parse(InfoNode *node, Rule *root) {
			parser->root(root);
			// TODO: Do our best to do error recovery when parsing like this!
			Str *src = node->toS();
			if (!parser->parse(src, path))
				return null;

			if (parser->matchEnd() != src->end())
				return null;

			InfoNode *r = parser->infoTree();
			parser->clear();
			return r;
		}

		Range Part::parse(const Range &range) {
			// TODO: We need some heurustic on how much we should try to re-parse in case we need
			// error recovery. We probably want to try a few parent nodes if error recovery has to
			// kick in near the leaves.
			// TODO: Limit the depth of what needs to be re-parsed in some situations.
			Range result;
			if (!parse(result, range, start, content)) {
				// Maybe we contain some parts of the previous part? Try to re-parse!
				owner->postInvalidate(this, -1, true);
			}
			return result;
		}

		Bool Part::parse(Range &result, const Range &range, Nat offset, InfoNode *node) {
			// We should probably parse at a higher level...
			Nat len = node->length();
			if (len == 0)
				return false;

			// Does this node completely cover 'range'?
			Range nodeRange(offset, offset + len);
			if (nodeRange.from > range.from || nodeRange.to < range.to)
				return false;

			// Do not attempt to re-parse leaf nodes. They only contain regexes, which have been
			// checked already.
			InfoInternal *inode = as<InfoInternal>(node);
			if (!inode)
				return false;


			// Re-parse this node if none of our children have already managed to do so.
			Bool ok = false;
			for (Nat i = 0; i < inode->count() && !ok; i++) {
				InfoNode *child = inode->at(i);
				ok |= parse(result, range, offset, child);
				offset += child->length();
			}

			if (!ok && inode->production()) {
				// Re-parse this node!
				InfoNode *n = parse(inode, inode->production()->rule());
				if (n) {
					replace(inode, n);
					result = nodeRange;
					ok = true;
				}
			}
			return ok;
		}

		void Part::replace(InfoNode *oldNode, InfoNode *newNode) {
			InfoInternal *parent = oldNode->parent();
			if (!parent) {
				assert(oldNode == content, L"A non-root node without a parent found.");
				content = newNode;
			} else {
				for (Nat i = 0; i < parent->count(); i++) {
					if (parent->at(i) == oldNode) {
						parent->set(i, newNode);
						return;
					}
				}

				assert(false, L"The child node was not present in our parent.");
			}
		}

		void Part::debugOutput(TextOutput *to, Bool tree) const {
			if (tree) {
				to->writeLine(content->format());
			} else {
				to->writeLine(content->toS());
			}
		}

		void Part::text(StrBuf *to) const {
			content->toS(to);
		}


		/**
		 * File.
		 */

		File::File(Nat id, Url *path, Str *src, WorkQueue *q)
			: id(id), path(path), work(q) {

			Engine &e = engine();
			parts = new (e) Array<Part *>();
			package = e.package(path->parent());
			if (!package) {
				WARNING(L"The file " << path << L" is not in a package known by Storm. Using a dummy package.");
				package = new (this) Package(new (this) Str(L"dummy package"));
			}

			PkgReader *pkgReader = createReader(new (this) Array<Url *>(1, path), package);
			FileReader *reader = null;
			if (pkgReader)
				reader = pkgReader->readFile(path, src);

			Nat offset = 0;
			while (reader) {
				FileReader *next = reader->next(qParser);
				Part *part = new (e) Part(this, offset, reader, next);
				offset = part->full().to;
				reader = next;
				parts->push(part);
			}
		}

		Range File::full() const {
			Range result;

			if (parts->any()) {
				result = parts->last()->full();
				result.from = 0;
			}

			return result;
		}

		Range File::replace(Range range, Str *replace) {
			// Total invalidated range.
			Range result;

			// The first part which needs to be invalidated eventually.
			Nat invalidateFrom = parts->count();
			Nat offset = 0;
			for (Nat i = 0; i < parts->count(); i++) {
				Part *part = parts->at(i);
				part->offset(offset);
				Range full = part->full();
				offset = full.to;

				Bool here = false;
				here |= range.intersects(full);
				// Note: 'range' can be empty, which causes problems when it lies right between two parts.
				here |= range.empty() && range.from == full.from;
				if (!here)
					continue;

				result = merge(result, part->replace(range, replace));
				invalidateFrom = min(invalidateFrom, i + 1);
			}

			// At the end of the last part?
			if (parts->any()) {
				Part *last = parts->last();
				if (last->full().to == range.from)
					result = merge(result, last->replace(range, replace));
			}

			// Shall we delay re-parsing of some parts?
			if (invalidateFrom < parts->count()) {
				work->post(new (this) InvalidatePart(this, invalidateFrom, false));
			}

			return result;
		}

		Array<ColoredRange> *File::colors(Range r) {
			Array<ColoredRange> *result = null;

			for (Nat i = 0; i < parts->count(); i++) {
				Part *part = parts->at(i);
				if (!r.intersects(part->full()))
					continue;

				Array<ColoredRange> *n = part->colors(r);
				if (result)
					result->append(n);
				else
					result = n;
			}

			if (!result)
				result = new (this) Array<ColoredRange>();

			return result;
		}

		void File::debugOutput(TextOutput *to, Bool tree) const {
			to->writeLine(TO_S(this, L"Content of " << id << L":"));

			for (Nat i = 0; i < parts->count(); i++) {
				parts->at(i)->debugOutput(to, tree);
				to->writeLine(new (this) Str(L"--------"));
			}
		}

		// Extract the text from the current state of all parts.
		static Str *content(Array<Part *> *parts) {
			StrBuf *out = new (parts) StrBuf();
			for (Nat i = 0; i < parts->count(); i++)
				parts->at(i)->text(out);
			return out->toS();
		}

		Range File::updatePart(Nat part, Bool force) {
			try {
				Range result;
				Str *src = content(parts);

				PkgReader *pkgReader = createReader(new (this) Array<Url *>(1, path), package);
				FileReader *reader = null;
				if (pkgReader)
					reader = pkgReader->readFile(path, src);

				// Note: we're currently ignoring that any previous parts could have changed their
				// range. This causes the parts generated to overlap a bit, which is actually not a
				// problem.
				Array<Part *> *newParts = new (this) Array<Part *>();
				while (reader) {
					FileReader *next = reader->next(qParser);
					Nat id = newParts->count();

					Nat offset = newParts->any() ? newParts->last()->full().to : 0;
					if (id < part) {
						Part *old = parts->at(id);
						old->offset(offset);
						newParts->push(old);
					} else if (id < parts->count() && !force) {
						Part *part = parts->at(id);
						part->offset(offset);
						if (part->replace(reader, next)) {
							result = merge(result, part->full());
						}
						newParts->push(part);
					} else {
						Part *part = new (this) Part(this, offset, reader, next);
						newParts->push(part);
						result = merge(result, part->full());
					}

					reader = next;
				}

				parts = newParts;

				return result;
			} catch (const Exception &) {
				// TODO: More narrow catch?
				return Range();
			}
		}

		void File::postInvalidate(Part *part, Int delta, Bool force) {
			for (Nat i = 0; i < parts->count(); i++) {
				if (part == parts->at(i)) {
					Nat r = i;
					if (delta >= 0)
						r += delta;
					else if (Nat(-delta) > r)
						r = 0;
					else
						r += delta;

					work->post(new (this) InvalidatePart(this, r, force));
				}
			}
		}


		/**
		 * Work items posted.
		 */

		InvalidatePart::InvalidatePart(File *file, Nat part, Bool force) : WorkItem(file), part(part), force(force) {}

		Range InvalidatePart::run() {
			return file->updatePart(part, force);
		}

		Bool InvalidatePart::equals(WorkItem *other) {
			if (!WorkItem::equals(other))
				return false;

			InvalidatePart *o = (InvalidatePart *)other;
			return part == o->part && force == o->force;
		}

	}
}
