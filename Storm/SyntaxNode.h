#pragma once
#include "SyntaxVariable.h"
#include "SyntaxOption.h"
#include "Std.h"

namespace storm {

	/**
	 * A syntax node. This is the result of parsing a string according to
	 * some grammar. Each node contains all captured variables from the
	 * rule in question. Everything else from the rule is discarded.
	 * The syntax node also contains a reference to the rule that created
	 * it. No ownership of the rule is taken, it is therefore important
	 * that the source rules remain in memory even though no more parsing
	 * is to be performed.
	 */
	class SyntaxNode : public Printable, NoCopy {
	public:
		SyntaxNode(const SyntaxOption *option);

		~SyntaxNode();

		// Add a value to the syntax node. Throws an error on failure.
		void add(const String &var, const String &val);
		void add(const String &var, SyntaxNode *node);

		// Add a method invocation to the syntax node. Throws an error on failure.
		void invoke(const String &member, const String &val);
		void invoke(const String &member, SyntaxNode *node);

		// Find the entry in the map for the variable. Creates the variable if it does not exist.
		SyntaxVariable *find(const String &name, SyntaxVariable::Type type);

		// Find the entry in the map for the variable. Returns null if the variable does not exist.
		SyntaxVariable *find(const String &name) const;

		// Reverse all arrays in this node (not recursive).
		void reverseArrays();

		// The syntax option that resulted in this node.
		const SyntaxOption *const option;

		// Get invocations.
		inline nat invocationCount() const { return invocations.size(); }
		inline std::pair<String, SyntaxVariable*> invocation(nat i) const { return invocations[i]; }

	protected:
		virtual void output(wostream &to) const;

	private:
		// All bound variables of this syntax node.
		typedef map<String, SyntaxVariable*> VarMap;
		VarMap vars;

		// Method invocations, in order.
		typedef std::pair<String, SyntaxVariable*> Invocation;
		vector<Invocation> invocations;

		// Find the type to use for this variable.
		SyntaxVariable::Type typeOf(const String &name, bool isString);

	};


	// Transform a syntax node into the representation described by
	// the original syntax rules and options. TODO: Type!
	Object *transform(Engine &e, const SyntaxNode &node);

}
