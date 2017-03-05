#include "stdafx.h"
#include "InfoIndent.h"
#include "Core/StrBuf.h"

namespace storm {
	namespace syntax {

		InfoIndent::InfoIndent(Nat start, Nat end, IndentType type) : start(start), end(end), type(type) {}

		Bool InfoIndent::contains(Nat i) const {
			return i >= start && i < end;
		}

		void InfoIndent::toS(StrBuf *to) const {
			*to << L"[" << start << L"-" << end << L"]" << type;
		}


		TextIndent::TextIndent() : value(0) {}

		Bool TextIndent::isAlign() const {
			return (value & relativeMask) != 0;
		}

		Int TextIndent::level() const {
			if (!isAlign()) {
				// Sign-extend to the last bit.
				if (value & (relativeMask >> 1))
					return value | relativeMask;
				else
					return value;
			} else {
				return 0;
			}
		}

		void TextIndent::level(Int v) {
			value = Nat(v) & ~relativeMask;
		}

		Nat TextIndent::alignAs() const {
			if (isAlign())
				return value & ~relativeMask;
			else
				return 0;
		}

		void TextIndent::alignAs(Nat offset) {
			value = offset | relativeMask;
		}

		void TextIndent::applyParent(InfoIndent *info, Nat offset) {
			switch (info->type) {
			case indentIncrease:
				if (!isAlign())
					level(level() + 1);
				break;
			case indentDecrease:
				if (!isAlign())
					level(level() - 1);
				break;
			case indentAlign:
				if (isAlign()) {
					// We only care about the leafmost one.
				} else {
					alignAs(offset);
				}
				break;
			}
		}

		void TextIndent::offset(Nat n) {
			if (isAlign())
				alignAs(alignAs() + n);
		}

		StrBuf &operator <<(StrBuf &to, TextIndent i) {
			if (i.isAlign())
				to << L"align as " << i.alignAs();
			else
				to << L"indent " << i.level() << L" levels";
			return to;
		}

		wostream &operator <<(wostream &to, TextIndent i) {
			if (i.isAlign())
				to << L"align as " << i.alignAs();
			else
				to << L"indent " << i.level() << L" levels";
			return to;
		}

		TextIndent indentLevel(Int level) {
			TextIndent i;
			i.level(level);
			return i;
		}

		TextIndent indentAs(Nat offset) {
			TextIndent i;
			i.alignAs(offset);
			return i;
		}

	}
}
