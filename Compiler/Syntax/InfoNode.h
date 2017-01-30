#pragma once
#include "Core/Str.h"
#include "Compiler/Thread.h"
#include "TokenColor.h"

namespace storm {
	namespace syntax {
		STORM_PKG(lang.bnf);

		class Production;
		class InfoInternal;
		class InfoLeaf;

		/**
		 * Syntax nodes containing all information about a parse. The main difference from 'Node' is
		 * that this representation contains all nodes, where 'Node' only contain the nodes the
		 * author of the syntax deemed interesting for generating further intermediate
		 * representations. This representation is also able to represent partial parses.
		 *
		 * A partial match is represented as usual, but the partial match may contain string nodes
		 * where an internal node would otherwise be expected. This represents failure to match a
		 * non-terminal.
		 *
		 * This representation is mainly intended for operations like syntax highlighting and
		 * indentation. It is also possible to incrementally update this representation when changes
		 * have been made to the source string since the nodes do not contain any absolute offsets
		 * into the original string.
		 */
		class InfoNode : public ObjectOn<Compiler> {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR InfoNode();

			// Specified color of this node.
			TokenColor color;

			// Length of this match (in codepoints).
			Nat STORM_FN length();

			// Find the first leaf node with a non-zero length at position 'pos' relative to this node.
			virtual MAYBE(InfoLeaf *) STORM_FN leafAt(Nat pos);

			// Format this info node into a human-readable representation.
			Str *STORM_FN format() const;

			// Generate the formatted string.
			virtual void STORM_FN format(StrBuf *to) const;

		protected:
			// Compute the length of this node.
			virtual Nat STORM_FN computeLength();

			// Invalidate any pre-computed data.
			void STORM_FN invalidate();

		private:
			// Cached length.
			Nat prevLength;
		};


		/**
		 * Internal node. Contains a fixed number of children.
		 */
		class InfoInternal : public InfoNode {
			STORM_CLASS;
		public:
			// Create and allocate space for a pre-defined number of child nodes. Make sure to
			// initialize all elements in the array before letting Storm access this node!
			InfoInternal(Production *prod, Nat children);

			// Get our production.
			inline Production *STORM_FN production() const { return prod; }

			// Number of children.
			inline Nat STORM_FN count() const {
				return children->count;
			}

			// Get child at offset.
			inline InfoNode *&STORM_FN operator [](Nat id) { return at(id); }
			InfoNode *&at(Nat id) {
				if (id < count()) {
					return children->v[id];
				} else {
					outOfBounds(id);
					return children->v[0];
				}
			}

			// Find the first leaf node at position 'pos' relative to this node.
			virtual MAYBE(InfoLeaf *) STORM_FN leafAt(Nat pos);

			// To string.
			virtual void STORM_FN toS(StrBuf *to) const;

			// Format.
			virtual void STORM_FN format(StrBuf *to) const;

		protected:
			// Compute the lenght of this node.
			virtual Nat STORM_FN computeLength();

		private:
			// Instance of which production?
			Production *prod;

			// Data.
			GcArray<InfoNode *> *children;

			// Throw out of bounds error.
			void outOfBounds(Nat id);
		};


		/**
		 * Leaf node. Contains the string matched at the leaf.
		 */
		class InfoLeaf : public InfoNode {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR InfoLeaf(Str *match);

			// Find leaf nodes.
			virtual MAYBE(InfoLeaf *) STORM_FN leafAt(Nat pos);

			// To string.
			virtual void STORM_FN toS(StrBuf *to) const;

			// Format.
			virtual void STORM_FN format(StrBuf *to) const;

		protected:
			// Compute the lenght of this node.
			virtual Nat STORM_FN computeLength();

		private:
			// The matched string.
			Str *v;
		};

	}
}
