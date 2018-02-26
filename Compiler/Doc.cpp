#include "stdafx.h"
#include "Doc.h"
#include "Named.h"
#include "Exception.h"
#include "Function.h"
#include "Variable.h"
#include "Type.h"
#include "Core/Join.h"

namespace storm {

	/**
	 * DocParam.
	 */

	DocParam::DocParam(Str *name, Value type) : name(name), type(type) {}

	StrBuf &operator <<(StrBuf &to, DocParam p) {
		if (p.name->any())
			return to << p.type << S(" ") << p.name;
		else
			return to << p.type;
	}

	wostream &operator <<(wostream &to, DocParam p) {
		if (p.name->any())
			return to << p.type << L" " << p.name;
		else
			return to << p.type;
	}


	/**
	 * DocNote.
	 */

	DocNote::DocNote(Str *note, Value type) : note(note), named(type.type), ref(type.ref) {}

	DocNote::DocNote(Str *note, Named *named) : note(note), named(named), ref(false) {}

	StrBuf &operator <<(StrBuf &to, DocNote n) {
		to << n.note << S(" ");
		if (n.named)
			to << n.named->identifier();
		else
			to << S("void");
		if (n.ref)
			to << S("&");
		return to;
	}

	wostream &operator <<(wostream &to, DocNote n) {
		to << n.note << L" ";
		if (n.named)
			to << n.named->identifier();
		else
			to << L"void";
		if (n.ref)
			to << L"&";
		return to;
	}

	static DocNote note(Engine &e, const wchar *note, Value type) {
		return DocNote(new (e) Str(note), type);
	}

	static DocNote note(Engine &e, const wchar *note, Named *named) {
		return DocNote(new (e) Str(note), named);
	}


	/**
	 * Doc.
	 */

	Doc::Doc(Str *name, Array<DocParam> *params, Str *body) :
		name(name), params(params), notes(new (engine()) Array<DocNote>()), body(body) {}

	Doc::Doc(Str *name, Array<DocParam> *params, DocNote note, Str *body) :
		name(name), params(params), notes(new (engine()) Array<DocNote>(1, note)), body(body) {}

	Doc::Doc(Str *name, Array<DocParam> *params, Array<DocNote> *notes, Str *body) :
		name(name), params(params), notes(notes), body(body) {}

	void Doc::deepCopy(CloneEnv *env) {
		cloned(name, env);
		cloned(params, env);
		cloned(notes, env);
		cloned(body, env);
	}

	void Doc::toS(StrBuf *to) const {
		*to << name;
		if (params->any())
			*to << S("(") << join(params, S(", ")) << S(")");
		if (notes->any())
			*to << S(" ") << join(notes, S(", "));
		*to << S(":\n\n");
		*to << body;
	}

	static Array<DocNote> *createNotes(Named *entity) {
		Engine &e = entity->engine();
		Array<DocNote> *notes = new (e) Array<DocNote>();

		if (Function *fn = as<Function>(entity)) {
			notes->push(note(e, S("->"), fn->result));
			RunOn on = fn->runOn();
			if (on.state != RunOn::any)
				notes->push(note(e, S("on"), on.thread));
		} else if (Variable *var = as<Variable>(entity)) {
			notes->push(note(e, S("->"), var->type));
		} else if (Type *type = as<Type>(entity)) {
			if (Type *super = type->super())
				notes->push(note(e, S("extends"), Value(super)));
			RunOn on = type->runOn();
			if (on.state != RunOn::any)
				notes->push(note(e, S("on"), on.thread));
		}

		return notes;
	}

	Doc *doc(Named *entity, Array<DocParam> *params, Str *body) {
		return new (entity) Doc(entity->name, params, createNotes(entity), body);
	}

	Doc *doc(Named *entity) {
		Array<DocParam> *params = new (entity) Array<DocParam>();
		for (Nat i = 0; i < entity->params->count(); i++)
			params->push(DocParam(new (entity) Str(S("?")), entity->params->at(i)));

		return new (entity) Doc(entity->name, params, createNotes(entity), new (entity) Str(S("")));
	}


	/**
	 * NamedDoc.
	 */

	NamedDoc::NamedDoc() {}

	Doc *NamedDoc::get() {
		throw InternalError(L"NamedDoc::get not overridden.");
	}

}
