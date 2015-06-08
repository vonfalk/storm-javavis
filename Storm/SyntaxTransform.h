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
	 * Note that if return parameters are derived from SObject, their 'pos' will be updated.
	 */

	// Transform the syntax tree. TODO: Static type-checking?
	Auto<Object> transform(Engine &e,
						SyntaxSet &syntax,
						const SyntaxNode &root,
						const vector<Object*> &params = vector<Object*>(),
						const SrcPos *pos = null);


	/**
	 * Represents an actual parameter, currently Object *, SrcPos and int is supported.
	 */
	class ActualBase : NoCopy {
	public:
		ActualBase() : refs(1) {}

		// Refcounting, so we can use it with 'Auto'.
		inline void addRef() {
			if (this)
				refs++;
		}

		inline void release() {
			if (this)
				if (--refs == 0)
					delete this;
		}

		// Add to fnParams.
		virtual void add(os::FnParams &to) const = 0;

		// Get the type.
		virtual Value type() const = 0;

	private:
		// # of references.
		nat refs;
	};

	template <class T>
	class Actual : public ActualBase {
	public:
		Actual(const T &v, const Value &type) : v(v), t(type) {}

		T v;

		Value t;

		void add(os::FnParams &to) const {
			to.add(v);
		}

		Value type() const {
			return t;
		}
	};

	class ActualObject : public ActualBase {
	public:
		ActualObject(const Auto<Object> &v, const SrcPos &pos);
		~ActualObject();

		Auto<Object> v;

		inline void add(os::FnParams &to) const {
			to.add<Object *>(v.borrow());
		}

		inline Value type() const {
			return Value(v->myType);
		}
	};


	/**
	 * Common variables in the entire transform.
	 */
	struct TransformEnv {
		Engine &e;
		SyntaxSet &syntax;
	};


	/**
	 * Keep track of variables during evaluation.
	 */
	class SyntaxVars : NoCopy {
	public:
		// Create.
		SyntaxVars(const SyntaxNode &node, const vector<Auto<ActualBase>> &params, TransformEnv &env, const SrcPos *pos);

		// Destroy.
		~SyntaxVars();

		// Get a variable by name.
		Auto<ActualBase> get(const String &name);

		// Set a variable by name.
		void set(const String &name, Auto<ActualBase> v);

	private:
		// Evaluated variables. Entries containing a null-pointer are currently being created.
		typedef hash_map<String, Auto<ActualBase>> Map;
		Map vars;

		// Syntax node.
		const SyntaxNode &node;

		// Context.
		TransformEnv &env;

		// Position
		const SrcPos *pos;

		// Add formal parameters to this rule.
		void addParams(vector<Auto<ActualBase>> params);

		// Compute the value of a variable.
		Auto<ActualBase> eval(const String &name);

		// Compute the value of 'v', when 'v' is considered not to be a known variable.
		Auto<ActualBase> valueOf(const String &v);
	};


	/**
	 * Type errors in the syntax somehow.
	 */
	class SyntaxTypeError : public CodeError {
	public:
		SyntaxTypeError(const SrcPos &pos, const String &msg) : CodeError(pos), msg(msg) {}

		inline String what() const { return ::toS(where) + L": " + msg; }
	private:
		String msg;
	};

}
