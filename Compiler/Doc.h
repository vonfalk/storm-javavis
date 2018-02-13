#pragma once
#include "Value.h"
#include "Core/Str.h"
#include "Utils/Exception.h"

namespace storm {
	STORM_PKG(core.lang);

	class Named;

	/**
	 * Parameter names for use in documentation.
	 */
	class DocParam {
		STORM_VALUE;
	public:
		// Create.
		STORM_CTOR DocParam(Str *name, Value type);

		// Parameter name.
		Str *name;

		// Parameter type.
		Value type;
	};

	// To string.
	StrBuf &STORM_FN operator <<(StrBuf &to, DocParam p);
	wostream &operator <<(wostream &to, DocParam p);

	/**
	 * Documentation for a single named entity.
	 *
	 * Instances of this class is generally not stored in memory longer than necessary. Rather, each
	 * 'Named' instance may provide a 'NamedDoc' instance that allows retrieving 'Doc' instances for
	 * that entity when needed. Documentation is generally browsed by a user, and as such it is
	 * desirable to load documentation on demand (since we can be fast enough) rather than preemptively.
	 *
	 * TODO: Include a reference to the Named or Template we are referring to?
	 * TODO: Include source position here or in the Named itself.
	 * TODO: Include a value that allows sorting the documentation for a type in some useful order.
	 */
	class Doc : public Object {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR Doc(Str *name, Array<DocParam> *params, Str *body);

		// Name of this entity.
		Str *name;

		// Parameters.
		Array<DocParam> *params;

		// Documentation body. Formatted using a format similar to Markdown.
		Str *body;

		// Deep copy.
		virtual void STORM_FN deepCopy(CloneEnv *env);

	protected:
		// To string.
		virtual void STORM_FN toS(StrBuf *to) const;
	};


	/**
	 * Documentation provider for named entities.
	 */
	class NamedDoc : public ObjectOn<Compiler> {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR NamedDoc();

		// Get documentation for this named entity.
		virtual Doc *STORM_FN get();
	};


	/**
	 * Exception for documentation errors.
	 */
	class EXCEPTION_EXPORT DocError : public Exception {
	public:
		inline DocError(const String &w) : w(w) {}
		inline virtual String what() const { return w; }
	private:
		String w;
	};


}
