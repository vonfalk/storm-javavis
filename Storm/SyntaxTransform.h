#pragma once
#include "SyntaxSet.h"
#include "SyntaxNode.h"
#include "SyntaxObject.h"
#include "Std.h"

namespace storm {

	/**
	 * Functions for transforming a syntax tree (represented by the root SyntaxNode)
	 * into the representation defined in the syntax definition.
	 *
	 * Note that return parameters has to be derived from core.lang.SObject.
	 */

	// Transform the syntax tree. TODO: Static type-checking?
	SObject *transform(Engine &e,
					SyntaxSet &syntax,
					const SyntaxNode &root,
					const vector<Object*> &params = vector<Object*>(),
					const SrcPos *pos = null);

	/**
	 * Used to keep track of currently evaluated variables during transform.
	 */
	class SyntaxVars : NoCopy {
	public:
		SyntaxVars(Engine &e, SyntaxSet &syntax, const SyntaxNode &node, const vector<Object*> &params);
		~SyntaxVars();

		SObject *get(const String &name);

		// Set a variable.
		void set(const String &name, Object *v);

	private:
		typedef hash_map<String, Object *> Map;
		Map vars;

		// Source node.
		const SyntaxNode &node;

		// Engine.
		Engine &e;

		// Syntax set
		SyntaxSet &syntax;

		// Keep track of infinite recursion in 'get'.
		hash_set<String> currentNames;

		// Cast to SObject.
		SObject *cast(Object *ptr);
	};
}
