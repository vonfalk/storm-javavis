#include "stdafx.h"
#include "Schema.h"

namespace sql {

	Schema::Schema(Str *tableName, Array<Column *> *columns)
		: tableName(tableName), columns(columns) {}

	void Schema::toS(StrBuf *to) const {
		*to << tableName << S(": {\n");
		to->indent();
		for (Nat i = 0; i < columns->count(); i++) {
			*to << columns->at(i) << S("\n");
		}
		to->dedent();
		*to << S("}");
	}

	Schema::Column::Column(Str *name, Str *dt)
		: name(name), datatype(dt), attributes(new (engine()) Array<Str *>()) {}

	Schema::Column::Column(Str *name, Str *dt, Array<Str *> *attrs)
		: name(name), datatype(dt), attributes(attrs) {}

	void Schema::Column::toS(StrBuf *to) const {
		*to << name << S(" ") << datatype;
		for (Nat i = 0; i < attributes->count(); i++) {
			*to << S(" ") << attributes->at(i);
		}
	}

}
