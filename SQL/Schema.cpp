#include "stdafx.h"
#include "Schema.h"
#include "Core/Join.h"

namespace sql {

	Schema::Schema(Str *tableName, Array<Column *> *columns)
		: tableName(tableName), columns(columns) {}

	Schema::Schema(Str *tableName, Array<Column *> *columns, Array<Str *> *pk)
		: tableName(tableName), columns(columns), pk(pk) {}

	Schema::Schema(Str *tableName, Array<Column *> *columns, Array<Str *> *pk, Array<Index *> *indices)
		: tableName(tableName), columns(columns), pk(pk), index(indices) {}

	Array<Str *> *Schema::primaryKeys() const {
		if (pk)
			return pk;
		else
			return new (this) Array<Str *>();
	}

	Array<Schema::Index *> *Schema::indices() const {
		if (index)
			return index;
		else
			return new (this) Array<Index *>();
	}

	void Schema::toS(StrBuf *to) const {
		*to << tableName << S(": {\n");
		to->indent();
		for (Nat i = 0; i < columns->count(); i++) {
			*to << columns->at(i) << S("\n");
		}
		if (pk) {
			*to << S("PRIMARY KEY(");
			*to << join(pk, S(", "));
			*to << S(")");
		}
		if (index) {
			for (Nat i = 0; i < index->count(); i++) {
				*to << index->at(i) << S("\n");
			}
		}
		to->dedent();
		*to << S("}");
	}

	Schema::Column::Column(Str *name, Str *dt)
		: name(name), datatype(dt), attributes(new (engine()) Str()) {}

	Schema::Column::Column(Str *name, Str *dt, Str *attrs)
		: name(name), datatype(dt), attributes(attrs) {}

	void Schema::Column::toS(StrBuf *to) const {
		*to << name << S(" ") << datatype;
		if (!attributes->empty())
			*to << S(" ") << attributes;
	}

	Schema::Index::Index(Str *name, Array<Str *> *columns)
		: name(name), columns(columns) {}

	void Schema::Index::toS(StrBuf *to) const {
		*to << S("INDEX ON ") << name << S("(") << join(columns, S(", ")) << S(")");
	}

}
