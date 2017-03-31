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
#include "WorkQueue.h"

namespace storm {
	namespace server {
		STORM_PKG(core.lang.server);

		class File;

		/**
		 * Represents a part of an open file in the language server. Corresponds to one of the parts
		 * in the chain of 'FileReader's.
		 */
		class Part : public ObjectOn<Compiler> {
			STORM_CLASS;
		public:
			STORM_CTOR Part(File *owner, Nat offset, FileReader *part, MAYBE(FileReader *) next);

			// Replaces the content in this part with the content dictated by 'reader'. Attempts to
			// re-parse the entire content, but keeps the old syntax tree in case parsing fails.
			// Returns 'false' if no replacement was needed.
			Bool STORM_FN replace(FileReader *part, MAYBE(FileReader *) next, Bool force);

			// Get the offset of this part.
			inline Nat STORM_FN offset() const { return start; }
			inline void STORM_FN offset(Nat offset) { start = offset; }

			// Adjust the offset of this part, modifying the content to match.
			void STORM_FN adjustOffset(Nat offset, Str *src);

			// Get the full range of this part.
			Range STORM_FN full() const;

			// Perform an edit to this part. Returns the amount of syntax information invalidated.
			Range STORM_FN replace(Range range, Str *replace);

			// Get syntax colorings in the specified range. These are ordered and
			// non-overlapping. The resulting colorings all overlap the specified range, but are not
			// clipped to be strictly inside the range.
			Array<ColoredRange> *colors(Range r);

			// Get the indentation of the character at offset 'n'.
			syntax::TextIndent indent(Nat pos);

			// Output our internal representation for debugging.
			void STORM_FN debugOutput(TextOutput *to, Bool tree) const;

			// Compute the total size of the syntax tree.
			Nat STORM_FN dbg_size() const;

			// Output all our text.
			void STORM_FN text(StrBuf *to) const;

		private:
			// Parser used for this part. Contains the proper packages etc.
			syntax::InfoParser *parser;

			// Parsed content of this part.
			syntax::InfoNode *content;

			// Path of this file. Only used for error reporting.
			Url *path;

			// Offset of the first character in this part.
			Nat start;

			// Owning file.
			File *owner;

			// Traverse the syntax tree and extract colors in 'range'.
			void colors(Array<ColoredRange> *out, const Range &range, Nat offset, syntax::InfoNode *node);

			// Re-parse a node in the syntax tree. Returns 'null' on complete failure.
			syntax::InfoNode *parse(syntax::InfoNode *node, syntax::Rule *root);
			syntax::InfoNode *parse(syntax::InfoNode *node, syntax::Rule *root, syntax::ParseResult *result);

			// Re-parse a range in the syntax tree. Returns the range that was re-parsed and needs
			// to be transmitted to the client again.
			Range parse(const Range &range);

			// State used in the recursive parse function.
			struct ParseEnv {
				// Range to re-parse.
				const Range update;

				// Range which has been modified and needs to be re-sent to the client.
				Range modified;

				// Range containing matches where error-correction kicked in.
				Range corrected;
			};

			// Traverse the parse tree from 'node' while attempting to update 'range', reparsing nodes as needed.
			// Returns the best parse result so far.
			syntax::ParseResult parse(ParseEnv &env, Nat offset, syntax::InfoNode *node);

			// Compare ParseResult:s and see which is better/worse.
			static bool better(syntax::ParseResult better, syntax::ParseResult than);
			static bool bad(syntax::ParseResult which, syntax::InfoNode *node);
			static syntax::ParseResult combine(syntax::ParseResult a, syntax::ParseResult b);

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
			STORM_CTOR File(Nat id, Url *path, Str *content, WorkQueue *q);

			// Get the range representing the entire file.
			Range STORM_FN full() const;

			// Perform an edit to this file. Returns how much of the syntax information was invalidated.
			Range STORM_FN replace(Range range, Str *replace);

			// Get syntax colorings in the specified range. These are ordered and
			// non-overlapping. The resulting colorings all overlap the specified range, but are not
			// clipped to be strictly inside the range.
			// TODO: Generate the required SExpr directly instead of going through an array.
			Array<ColoredRange> *colors(Range r);

			// Get the indentation of the character at offset 'n'.
			syntax::TextIndent indent(Nat pos);

			// Our id.
			Nat id;

			// ID of the last edit operation.
			Nat editId;

			// Position of last edit operation (only a hint).
			Nat editPos;

			// Output our internal representation for debugging.
			void STORM_FN debugOutput(TextOutput *to, Bool tree) const;

			// Post a message for invalidating a part (or before/after a part).
			void postInvalidate(Part *part, Int offset, Bool force);

		private:
			// Path to the underlying file (so we can properly locate packages etc., we never actually read the file).
			Url *path;

			// Remember the package this file is located inside.
			Package *package;

			// All parts.
			Array<Part *> *parts;

			// Work queue.
			WorkQueue *work;

			// Update 'part', return the modified range. This generally causes all following parts
			// to be updated eventually as well.
			Range updatePart(Nat part, Bool force);

			friend class InvalidatePart;
		};


		/**
		 * Work items posted when we need to invalidate one or more parts.
		 */
		class InvalidatePart : public WorkItem {
			STORM_CLASS;
		public:
			STORM_CTOR InvalidatePart(File *file, Nat part, Bool force);

			Nat part;
			Bool force;

			virtual Range STORM_FN run(WorkQueue *q);
			virtual Bool STORM_FN merge(WorkItem *o);
		};

	}
}
