#include "stdafx.h"
#include "Doc.h"
#include "Function.h"
#include "Type.h"
#include "Class.h"
#include "Compiler/Exception.h"
#include "Core/Io/Text.h"
#include "Core/Io/Stream.h"

namespace storm {
	namespace bs {

		struct State {
			enum S {
				start,
				start2,
				done,

				singleStart,
				singleInside,
				singleNewline,
				singleHalf,
				singleBefore,

				multiStart,
				multiInside,
				multiNewline,
				multiBefore,
			};

			// State.
			S state;

			// Number of spaces after the comment.
			Nat space;

			// Current number of spaces.
			Nat curr;

			// Number of empty lines encountered.
			Nat empty;
		};

		static bool whitespace(Char ch) {
			switch (ch.codepoint()) {
			case ' ':
			case '\n':
			case '\r':
			case '\t':
				return true;
			default:
				return false;
			}
		}

		static const wchar *parseBody(StrBuf *out, State &s, Char ch) {
			switch (s.state) {
			case State::start:
				// Find a '/'.
				if (ch == Char('/')) {
					s.state = State::start2;
					return null;
				}

				if (whitespace(ch))
					return null;
				return S("Could not find the start of the comment.");
			case State::start2:
				// Either * or /
				if (ch == Char('*'))
					s.state = State::multiStart;
				else if (ch == Char('/'))
					s.state = State::singleStart;
				else
					return S("Unknown comment type.");
				return null;

			case State::singleStart:
				if (whitespace(ch)) {
					s.space++;
				} else {
					*out << ch;
					s.state = State::singleInside;
				}
				return null;
			case State::singleInside:
				if (ch == Char('\n')) {
					s.curr = 0;
					s.empty = 1;
					s.state = State::singleNewline;
				} else {
					*out << ch;
				}
				return null;
			case State::singleNewline:
				if (ch == Char('/'))
					s.state = State::singleHalf;
				else if (!whitespace(ch))
					s.state = State::done;
				return null;
			case State::singleHalf:
				if (ch == Char('/'))
					s.state = State::singleBefore;
				else
					s.state = State::done;
				return null;
			case State::singleBefore:
				if (ch == Char(' ')) {
					s.curr++;
				} else if (ch == Char('\n')) {
					s.empty++;
					s.curr = 0;
					s.state = State::singleNewline;
				} else if (!whitespace(ch)) {
					while (s.empty) {
						*out << Char('\n');
						s.empty--;
					}

					if (s.curr < s.space)
						return S("Not enough indentation after //");
					for (Nat i = s.space; i < s.curr; i++)
						*out << Char(' ');
					s.curr = 0;

					*out << ch;
					s.state = State::singleInside;
				}
				return null;


			case State::multiStart:
				if (ch == Char('*')) {
					s.space = 0;
				} else if (whitespace(ch)) {
					s.space++;
				} else {
					*out << ch;
					s.state = State::multiInside;
				}
				return null;
			case State::multiInside:
				if (ch == Char('\n')) {
					s.curr = 0;
					s.empty = 1;
					s.state = State::multiNewline;
				} else {
					*out << ch;
				}
				return null;
			case State::multiNewline:
				if (ch == Char('*'))
					s.state = State::multiBefore;
				else if (!whitespace(ch))
					s.state = State::done;
				return null;
			case State::multiBefore:
				if (s.curr == 0 && ch == Char('/')) {
					s.state = State::done;
				} else if (ch == Char(' ')) {
					s.curr++;
				} else if (ch == Char('\n')) {
					s.empty++;
					s.curr = 0;
					s.state = State::multiNewline;
				} else if (!whitespace(ch)) {
					while (s.empty) {
						*out << Char('\n');
						s.empty--;
					}

					if (s.curr < s.space)
						return S("Not enough indentation after the first *");
					for (Nat i = s.space; i < s.curr; i++)
						*out << Char(' ');
					s.curr = 0;

					*out << ch;
					s.state = State::multiInside;
				}
				return null;
				}

			return S("Internal error.");
		}

		static Str *parseBody(SrcPos pos, TextInput *src) {
			StrBuf *out = new (src) StrBuf();
			State state = { State::start, 0, 0, 0 };

			while (state.state != State::done && src->more()) {
				Char ch = src->read();
				if (ch == Char('\r'))
					continue;

				if (const wchar *msg = parseBody(out, state, ch)) {
					*out << S("\n\n<incomplete comment: ") << msg << S(" Is the source changed?>");
					state.state = State::done;
				}
			}

			if (state.state != State::done)
				*out << S("\n\n<incomplete comment>");

			return out->toS();
		}


		BSDoc::BSDoc(SrcPos docPos, Named *entity) : docPos(docPos), entity(entity) {}

		Doc *BSDoc::get() {
			Str *body = readBody();
			Array<DocParam> *params = createParams();

			return doc(entity, params, body);
		}

		Str *BSDoc::readBody() {
			if (!docPos.file)
				return new (this) Str(S(""));

			IStream *s = docPos.file->read();
			TextInput *src = readText(s);

			// Skip until we reach the indicated position.
			Nat pos = 0;
			while (src->more() && pos < docPos.start) {
				src->read();
				pos++;
			}

			if (pos != docPos.start) {
				s->close();
				Str *msg = TO_S(engine(), S("Unable to find a comment in ") << docPos << S(". Is the file changed?"));
				throw new (this) DocError(msg);
			}

			try {
				Str *body = parseBody(docPos, src);
				s->close();
				return body;
			} catch (...) {
				s->close();
				throw;
			}
		}

		Array<DocParam> *BSDoc::createParams() {
			if (BSRawFn *bs = as<BSRawFn>(entity)) {
				return bs->docParams();
			} else {
				Str *empty = new (this) Str(S(""));
				Array<DocParam> *r = new (this) Array<DocParam>();
				r->reserve(entity->params->count());

				for (Nat i = 0; i < entity->params->count(); i++)
					r->push(DocParam(empty, entity->params->at(i)));

				return r;
			}
		}


		TObject *applyDoc(SrcPos pos, TObject *to) {
			if (Named *named = as<Named>(to)) {
				named->documentation = new (named) BSDoc(pos, named);
			} else if (NamedDecl *decl = as<NamedDecl>(to)) {
				// Tell the declaration where the documentation is. It will call us again later so
				// that we can set the documentation properly.
				decl->docPos = pos;
			} else if (MemberWrap *wrap = as<MemberWrap>(to)) {
				// Remember where the documentation is. It will call us again later so that we can
				// set the documentation properly.
				wrap->docPos = pos;
			} else {
				// TODO: Handle documentation for generators as well!
				// PVAR(runtime::typeOf(to)->identifier());
			}


			return to;
		}

	}
}
