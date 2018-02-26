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
	 * Documentation note. Contains a type and a string that is to displayed alongside the name of
	 * the object. Used for things like return types and inheritance.
	 */
	class DocNote {
		STORM_VALUE;
	public:
		// Create.
		STORM_CTOR DocNote(Str *note, Value type);
		STORM_CTOR DocNote(Str *note, Named *named);

		// Note.
		Str *note;

		// Named object.
		MAYBE(Named *) named;

		// Reference?
		Bool ref;
	};

	// To string.
	StrBuf &STORM_FN operator <<(StrBuf &to, DocNote n);
	wostream &operator <<(wostream &to, DocNote n);


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
		STORM_CTOR Doc(Str *name, Array<DocParam> *params, DocNote note, Str *body);
		STORM_CTOR Doc(Str *name, Array<DocParam> *params, Array<DocNote> *notes, Str *body);

		// Name of this entity.
		Str *name;

		// Parameters.
		Array<DocParam> *params;

		// Notes.
		Array<DocNote> *notes;

		// Documentation body. Formatted using a format similar to Markdown.
		Str *body;

		// Deep copy.
		virtual void STORM_FN deepCopy(CloneEnv *env);

	protected:
		// To string.
		virtual void STORM_FN toS(StrBuf *to) const;
	};

	// Create documentation, infer 'name' and 'notes' from an entity.
	Doc *STORM_FN doc(Named *entity, Array<DocParam> *params, Str *body) ON(Compiler);

	// Create a dummy documentation block for 'entity'. Used when no NamedDoc is provided.
	Doc *STORM_FN doc(Named *entity) ON(Compiler);


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
