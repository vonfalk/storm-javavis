#pragma once

namespace sql {

	/**
	 * The Schema class contains the structure from a table within your database.
	 * To get the Schema one would need to call the schema() function in the SQLite class.
	 *
	 * TODO: This is implemented inside SQLite.cpp and has things quite tightly tied to that
	 * database. Generalize it!
	 *
	 * TODO: The nomenclature is a bit off as well (e.g. "rows", but should be "columns").
	 */
	class Schema : public Object {
		STORM_CLASS;
	public:

		//Default constructor of a Schema Class.
		STORM_CTOR Schema();

		//Returns the row size of the table.
		Nat STORM_FN size() const;

		//Returns the table name of a table.
		Str* STORM_FN getTable() const;

		/**
		 * Schema contains the content class, which holds the name, datatype and all attributes for
		 * a given row.  This row is specified in the getRow function from the Schema class.
		 */
		class Content : public Object {
			STORM_CLASS;
		public:

			//Constructors of the Content class, either empty or with set parameters
			STORM_CTOR Content();
			STORM_CTOR Content(Str* name, Str* dt, Array<Str*> *attributes);

			//Getters for the Content class
			Str * STORM_FN getName() const;
			Str * STORM_FN getDt() const;
			Array<Str*> * STORM_FN getAttr() const;

			void STORM_FN setAttr(Array<Str*>* attr);

		private:
			Str* name;
			Str* dt;
			Array<Str*> * attributes;
		};

		STORM_CTOR Schema(Array<Content*> *row, Str *table_name);

		//Get the name, datatype and attribute for a row(actually a column) from your table schema.
		Content * STORM_FN getRow(Int idx) const;

		void STORM_FN getForeignKey(Str * str, Array<Content*> * row);
		Str * STORM_FN removeIndent(Str * str);

	private:
		Array<Content*>* row;
		Str * table_name;
	};

}
