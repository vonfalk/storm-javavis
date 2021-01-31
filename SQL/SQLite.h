#pragma once
#include "SQLite/sqlite3.h"
#include "Core/Io/Url.h"
#include "Core/Array.h"
#include "Core/Variant.h"
#include "SQL.h"

namespace sql {

	class SQLite;

	/**
	 * A class representing a SQLite statement.
	 */
	class SQLite_Statement : public Statement {
		STORM_CLASS;
	public:


		// Constructor of an SQLite_Statement. Takes an SQLite database and an Str.
		STORM_CTOR SQLite_Statement(const SQLite *database, Str *str);

		// Destructor of an SQLite_Statement. Calls SQLite_Statement:finalize().
		~SQLite_Statement();


		// Binds for all relevant data types in SQLite. If a statement includes question marks,
		// these are used to bind one string to a question mark.
		// pos specifies which question mark is to be replaced by matched data type.
		void STORM_FN bind(Nat pos, Str *str) override;
		void STORM_FN bind(Nat pos, Int i) override;
		void STORM_FN bind(Nat pos, Double d) override;

		// Executes an SQLite statement, returns true if execute was successful.
		void STORM_FN execute() override;

		// Calls SQLite3_finalize on SQLite_Statement and cleans member variables.
		void STORM_FN finalize() override;

		// Calls SQLite3_reset on SQLite_Statement.
		void STORM_FN reset() override;

		// Fetches a new row and returns nullptr if result is false or SQLITE_DONE-flag is set.
		MAYBE(Row *) STORM_FN fetch() const override;
		Long STORM_FN lastRowId() const override;

	private:
		Bool result;
		sqlite3_stmt * stmt;  // Used for prepared statements
		const SQLite *db;
	};

	/**
	 * Database class specificly for SQLite.
	 */
	class SQLite : public Database {
		STORM_CLASS;
	public:

		// Destructor of an SQLite database connection. Calls SQLite:close().
		virtual ~SQLite();

		// Constructors of an SQLite database connection. either connected through a file
		// on given URL or is created in memory.
		STORM_CTOR SQLite(Url * str);
		STORM_CTOR SQLite();

		// Returns an SQLite_Statement given an Str str.
		Statement * STORM_FN prepare(Str *str) override;

		// Returns last row id of a connection.
		Long STORM_FN lastRowId() const override;

		// Calls sqlite3_close(db).
		void STORM_FN close() override;

		// Returns all names of tables in SQLite connection in an Array of Str.
		Array<Str*>* STORM_FN tables() override;

		// Getter for member variable db.
		sqlite3 * raw() const;

		SQLite * STORM_FN getDB();

		// Returns a Schema for SQLite connection.
		Schema * STORM_FN schema(Str *str);

	private:
		sqlite3 * db;
	};

}
