#pragma once
#include "Core/Array.h"
#include "Core/Str.h"

namespace sql {

	/**
	 * The Schema class contains the structure from a table within your database.
	 * To get the Schema one would need to call the schema() function in the SQLite class.
	 */
	class Schema : public Object {
		STORM_CLASS;
	public:
		/**
		 * Schema contains the content class, which holds the name, datatype and all attributes for
		 * a given row.  This row is specified in the getRow function from the Schema class.
		 */
		class Column : public Object {
			STORM_CLASS;
		public:
			// Constructors of the Column class.
			STORM_CTOR Column(Str *name, Str *dt);
			STORM_CTOR Column(Str *name, Str *dt, Str *attributes);

			// Name of the column.
			Str *name;

			// Datatype of the column.
			Str *datatype;

			// Attributes for the column. These are not parsed or separated (that requires intricate
			// understanding of the SQL syntax).
			Str *attributes;

		protected:
			// To string.
			virtual void STORM_FN toS(StrBuf *to) const override;
		};

		// Create a filled schema.
		STORM_CTOR Schema(Str *tableName, Array<Column *> *columns);
		STORM_CTOR Schema(Str *tableName, Array<Column *> *columns, Array<Str *> *pk);

		// Number of columns in this table.
		Nat STORM_FN count() const {
			return columns->count();
		}

		// The name of this table.
		Str *STORM_FN name() const {
			return tableName;
		}

		// Get a column.
		Column *STORM_FN at(Nat id) const {
			return columns->at(id);
		}
		Column *STORM_FN operator[](Nat id) const {
			return columns->at(id);
		}

		// Get primary keys.
		Array<Str *> *STORM_FN primaryKeys() const;

	protected:
		// To string.
		virtual void STORM_FN toS(StrBuf *to) const override;

	private:
		// Name of this table.
		Str *tableName;

		// Columns.
		Array<Column *> *columns;

		// Primary keys (declared separatly).
		Array<Str *> *pk;
	};

}
