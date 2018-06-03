#include "stdafx.h"
#include "NamedSource.h"
#include "Named.h"

namespace storm {

	NamedSource::NamedSource(Named *entity) : RefSource(), entity(entity), subtype(Nat(0)) {}

	NamedSource::NamedSource(Named *entity, Char subtype) : RefSource(), entity(entity), subtype(subtype) {}

	Str *NamedSource::title() const {
		Str *result = entity->identifier();
		if (subtype != Char(Nat(0))) {
			StrBuf *buf = new (this) StrBuf();
			*buf << result << S("<") << subtype << S(">");
			result = buf->toS();
		}
		return result;
	}

}
