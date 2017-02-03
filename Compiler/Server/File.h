#pragma once
#include "Core/Str.h"
#include "Core/Io/Url.h"
#include "Core/Io/Text.h"
#include "Core/EnginePtr.h"
#include "Compiler/Thread.h"
#include "Compiler/Reader.h"
#include "Compiler/FileReader.h"
#include "Compiler/Syntax/TokenColor.h"
#include "Compiler/Syntax/InfoNode.h"

namespace storm {
	namespace server {
		STORM_PKG(core.lang.server);

		/**
		 * Represents a range in a file.
		 */
		class Range {
			STORM_VALUE;
		public:
			STORM_CTOR Range();
			STORM_CTOR Range(Nat from, Nat to);

			Nat from;
			Nat to;

			// Is this range empty?
			inline Bool STORM_FN empty() const { return from == to; }

			// Does this range intersect another range?
			Bool STORM_FN intersects(Range other) const;
		};

		StrBuf &STORM_FN operator <<(StrBuf &to, Range r);

		// Compute the union of two ranges.
		Range STORM_FN combine(Range a, Range b);

		/**
		 * Represents a a region to be colored in a file.
		 */
		class ColoredRange {
			STORM_VALUE;
		public:
			STORM_CTOR ColoredRange(Range r, syntax::TokenColor c);

			Range range;

			// TODO: Use strings or symbols here?
			syntax::TokenColor color;
		};

		StrBuf &STORM_FN operator <<(StrBuf &to, ColoredRange r);

		/**
		 * Represents an open file in the language server.
		 *
		 * TODO: We need a more efficient representation of the contents!
		 */
		class File : public ObjectOn<Compiler> {
			STORM_CLASS;
		public:
			STORM_CTOR File(Nat id, Url *path, Str *content);

			// Get the range representing the entire file.
			Range STORM_FN full();

			// Perform an edit to this file. Returns how much of the syntax information was invalidated.
			Range STORM_FN replace(Range range, Str *replace);

			// Get all syntax colorings in the specified range. These are ordered and non-overlapping.
			Array<ColoredRange> *colors(Range r);

			// Our id.
			Nat id;

			// Output our internal representation for debugging.
			void STORM_FN debugOutput(TextOutput *to, Bool tree) const;

		private:
			// Path to the underlying file (so we can properly locate packages etc., we never actually read the file).
			Url *path;

			// Parser used for this file.
			syntax::InfoParser *parser;

			// File content (parsed).
			syntax::InfoNode *content;

			// Traverse the syntax tree and extract colors in 'range'.
			void colors(Array<ColoredRange> *out, const Range &range, Nat offset, syntax::InfoNode *node);

			// Re-parse a node in the syntax tree. Returns 'null' on complete failure.
			syntax::InfoNode *parse(syntax::InfoNode *node, syntax::Rule *root);

			// Re-parse a range in the syntax tree. Returns the range that was re-parsed.
			Range parse(const Range &range);

			// Helpers for 'parse'.
			Bool parse(Range &result, const Range &range, Nat offset, syntax::InfoNode *node);

			// Remove 'range' in the current node (if applicable). The structure of the syntax tree
			// is kept intact. Returns 'false' if content was modified so that a regex in a leaf node
			// no longer matches the contents of that node (or if a modified leaf node is missing a regex).
			Bool remove(const Range &range);

			// Helpers to 'remove'
			Bool remove(const Range &range, Nat offset, syntax::InfoNode *node);
			Bool removeLeaf(const Range &range, Nat offset, syntax::InfoLeaf *src);
			Bool removeInternal(const Range &range, Nat offset, syntax::InfoInternal *node);

			// State struct used in 'insert'.
			struct InsertState;

			// Insert 'v' at 'point'. Does not alter the structure of the tree. Returns 'false' if a
			// node was modified so that its regex no longer matches its content.
			Bool insert(Nat point, Str *v);

			// Helpers to 'insert'.
			void insert(InsertState &state, Nat offset, syntax::InfoNode *node);
			void insertLeaf(InsertState &state, Nat offset, syntax::InfoLeaf *node);
			void insertInternal(InsertState &state, Nat offset, syntax::InfoInternal *node);

			// Replace node 'oldNode' with 'newNode' somewhere in the tree.
			void replace(syntax::InfoNode *oldNode, syntax::InfoNode *newNode);
		};

	}
}
