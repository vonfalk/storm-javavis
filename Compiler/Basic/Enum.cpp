#include "stdafx.h"
#include "Enum.h"
#include "Compiler/Exception.h"

namespace storm {
	namespace bs {

		Enum *createEnum(syntax::SStr *name, MAYBE(Str *) bitmask) {
			return new (name) Enum(name->v, bitmask != null);
		}

		EnumContext *context(Enum *e) {
			return new (e) EnumContext(e);
		}

		EnumContext::EnumContext(Enum *e) : e(e), overflow(false) {
			if (e->bitmask())
				next = 1;
			else
				next = 0;
		}

		EnumValue *EnumContext::create(syntax::SStr *name) {
			if (overflow)
				throw new (this) SyntaxError(name->pos, S("The value for this label would overflow a 32-bit integer."));

			return create(name, next);
		}

		EnumValue *EnumContext::create(syntax::SStr *name, Nat value) {
			if (e->bitmask()) {
				if (value & (1 << (sizeof(Nat)*CHAR_BIT - 1)))
					overflow = true;
				else
					next = nextPowerOfTwo(value + 1);
			} else {
				next = value + 1;
				// This is safe, since 'Nat' is unsigned.
				if (next < value)
					overflow = true;
			}

			EnumValue *created = new (this) EnumValue(e, name->v, value);
			e->add(created);
			return created;
		}

	}
}
