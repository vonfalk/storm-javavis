#pragma once
#include "Core/Str.h"
#include "Core/Io/Url.h"
#include "Core/Io/Text.h"
#include "Core/EnginePtr.h"
#include "Compiler/Thread.h"
#include "Compiler/Reader.h"
#include "Compiler/FileReader.h"
#include "Compiler/Syntax/InfoNode.h"
#include "Range.h"

namespace storm {
	namespace server {
		STORM_PKG(core.lang.server);

		/**
		 * Represents a part of an open file in the language server. Corresponds to one of the parts
		 * in the chain of 'FileReader's.
		 */
		class Part : public ObjectOn<Compiler> {
			STORM_CLASS;
		public:
			STORM_CTOR Part(Nat offset, FileReader *part, MAYBE(FileReader *) next);

			// Get the offset of this part.
			inline Nat STORM_FN offset() const { return start; }

			// Get the full range of this part.
			Range STORM_FN full() const;

			// Perform an edit to this part. Returns the amount of syntax information invalidated.
			Range STORM_FN replace(Range range, Str *replace);

			// Get syntax colorings in the specified range. These are ordered and
			// non-overlapping. The resulting colorings all overlap the specified range, but are not
			// clipped to be strictly inside the range.
			Array<ColoredRange> *colors(Range r);

			// Output our internal representation for debugging.
			void STORM_FN debugOutput(TextOutput *to, Bool tree) const;

		private:
			// Parser used for this part. Contains the proper packages etc.
			syntax::InfoParser *parser;

			// Parsed content of this part.
			syntax::InfoNode *content;

			// Path of this file. Only used for error reporting.
			Url *path;

			// Offset of the first character in this part.
			Nat start;

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

		/**
		 * Represents an open file in the language server.
		 */
		class File : public ObjectOn<Compiler> {
			STORM_CLASS;
		public:
			STORM_CTOR File(Nat id, Url *path, Str *content);

			// Get the range representing the entire file.
			Range STORM_FN full() const;

			// Perform an edit to this file. Returns how much of the syntax information was invalidated.
			Range STORM_FN replace(Range range, Str *replace);

			// Get syntax colorings in the specified range. These are ordered and
			// non-overlapping. The resulting colorings all overlap the specified range, but are not
			// clipped to be strictly inside the range.
			Array<ColoredRange> *colors(Range r);

			// Our id.
			Nat id;

			// ID of the last edit operation.
			Nat editId;

			// Output our internal representation for debugging.
			void STORM_FN debugOutput(TextOutput *to, Bool tree) const;

		private:
			// Path to the underlying file (so we can properly locate packages etc., we never actually read the file).
			Url *path;

			// All parts.
			Array<Part *> *parts;
		};

	}
}
