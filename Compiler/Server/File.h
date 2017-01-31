#pragma once
#include "Core/Str.h"
#include "Core/Io/Url.h"
#include "Core/Io/Text.h"
#include "Core/EnginePtr.h"
#include "Compiler/Thread.h"
#include "Compiler/Reader.h"
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
			STORM_CTOR Range(Nat from, Nat to);

			Nat from;
			Nat to;

			// Is this range empty?
			inline Bool STORM_FN empty() const { return from == to; }

			// Does this range intersect another range?
			Bool STORM_FN intersects(Range other) const;
		};

		StrBuf &STORM_FN operator <<(StrBuf &to, Range r);

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
		 * Represents a node in the syntax tree together with its depth, so we can quickly decide
		 * which is the most shallow node.
		 */
		class NodePtr {
			STORM_VALUE;
		public:
			// Create a null pointer.
			STORM_CTOR NodePtr();

			// Create a pointer to a node and remember its depth.
			STORM_CTOR NodePtr(syntax::InfoNode *node, Nat depth);

			// Node.
			syntax::InfoNode *node;

			// Depth.
			Nat depth;
		};

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

			// Re-parse a node in the syntax tree.
			syntax::InfoNode *parse(syntax::InfoNode *node, syntax::Rule *root);

			// Traverse the syntax tree and extract colors in 'range'.
			void colors(Array<ColoredRange> *out, const Range &range, Nat offset, syntax::InfoNode *node);

			// Remove 'range' in the current node (if applicable). The structure of the syntax tree
			// is kept intact. Returns the rootmost node that was altered.
			void remove(const Range &range, Nat offset, syntax::InfoNode *node);
			void removeLeaf(const Range &range, Nat offset, syntax::InfoLeaf *src);
			void removeInternal(const Range &range, Nat offset, syntax::InfoInternal *node);

			// Insert 'v' at 'point'. Does not alter the structure of the tree. Returns the rootmost
			// node that was modified.
			syntax::InfoNode *insert(Nat point, Str *v, Nat offset, syntax::InfoNode *node);
		};

	}
}
