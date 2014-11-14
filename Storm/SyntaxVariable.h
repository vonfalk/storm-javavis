#pragma once
#include "Exception.h"

namespace storm {

	class SyntaxNode;

	/**
	 * Represents a bound variable in a syntax. A syntax variable is either:
	 * - A string, when bound to a regex.
	 * - A SyntaxNode, when bound to other syntax.
	 * - An array of either string or syntaxes.
	 */
	class SyntaxVariable : public Printable, NoCopy {
	public:
		enum Type {
			tString, tStringArr,
			tNode, tNodeArr,
		};

		// Create a syntax variable with a given type.
		SyntaxVariable(Type type);

		// Dtor.
		~SyntaxVariable();

		// Add various types to this variable. These throw an error on failure.
		// TODO? Enforce SrcRef for these?
		void add(const String &str);
		void add(SyntaxNode *node);

		// Which type?
		const Type type;

		// Get the name of 'type'.
		static String name(Type t);

		// Get different types. Asserts on failure!
		inline const String &string() const { assert(type == tString); return *str; }
		inline const vector<String> &stringArr() const { assert(type == tStringArr); return *strArr; }
		inline SyntaxNode *node() const { assert(type == tNode); return n; }
		inline const vector<SyntaxNode*> &nodeArr() const { assert(type == tNodeArr); return *nArr; }

		// Orphan any owned data.
		void orphan();

		// Reverse the array (if we're an array).
		void reverseArray();

	protected:
		virtual void output(wostream &to) const;

	private:
		// Different data types.
		union {
			String *str;
			vector<String> *strArr;
			SyntaxNode *n;
			vector<SyntaxNode*> *nArr;
		};

	};


	/**
	 * Error thrown when expected type does not match actual type. This is
	 * designed so that the error position can be filled in later.
	 */
	class SyntaxTypeError : public CodeError {
	public:
		SyntaxTypeError(const String &msg) : CodeError(SrcPos()), msg(msg) {}
		SyntaxTypeError(const String &msg, const SrcPos &pos) : CodeError(pos), msg(msg) {}

		inline String what() const { return toS(where) + L": Invalid types: " + msg; }
	private:
		String msg;
	};

}
