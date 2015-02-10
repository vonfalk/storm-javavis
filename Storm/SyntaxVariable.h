#pragma once
#include "Exception.h"

namespace storm {

	class SyntaxNode;

	/**
	 * Represents a bound variable in a syntax. A syntax variable is either:
	 * - A string, when bound to a regex.
	 * - A SyntaxNode, when bound to other syntax.
	 */
	class SyntaxVariable : public Printable, NoCopy {
	public:
		enum Type {
			tString,
			tNode,
		};

		// Create a syntax variable with a given type.
		SyntaxVariable(const SrcPos &pos, const String &v);
		SyntaxVariable(const SrcPos &pos, SyntaxNode *node);

		// Dtor.
		~SyntaxVariable();

		// Which type?
		const Type type;

		// Get the name of 'type'.
		static String name(Type t);

		// Source
		SrcPos pos;

		// Get different types. Asserts on failure!
		inline const String &string() const { assert(type == tString); return *str; }
		inline SyntaxNode *node() const { assert(type == tNode); return n; }

		// Orphan any owned data.
		void orphan();

	protected:
		virtual void output(wostream &to) const;

	private:
		// Different data types.
		union {
			String *str;
			SyntaxNode *n;
		};

	};

}
