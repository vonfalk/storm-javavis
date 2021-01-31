#include "stdafx.h"
#include "SQLite.h"
#include "Exception.h"

namespace sql {


	/////////////////////////////////////
	//			  Statement			   //
	/////////////////////////////////////

	SQLite_Statement::SQLite_Statement(const SQLite *database, Str *str) {
		db = database;
		result = false;
		lastId = 0;
		int ok = sqlite3_prepare_v2(db->raw(), str->utf8_str(), -1, &stmt, null);
		if (ok != SQLITE_OK) {
			Str *msg = new (this) Str((wchar *)sqlite3_errmsg16(db->raw()));
			throw new (this) SQLError(msg);
		}
	}

	SQLite_Statement::~SQLite_Statement() {
		finalize();
	}

	void SQLite_Statement::bind(Nat pos, Str *str) {
		sqlite3_bind_text(stmt, pos + 1, str->utf8_str(), -1, SQLITE_TRANSIENT);
	}

	void SQLite_Statement::bind(Nat pos, Int i) {
		sqlite3_bind_int(stmt, pos + 1, (int)i);
	}
	void SQLite_Statement::bind(Nat pos, Double d) {
		sqlite3_bind_double(stmt, pos + 1, (double)d);
	}

	void SQLite_Statement::execute() {
		Int rc = sqlite3_step(stmt);
		lastId = sqlite3_last_insert_rowid(db->raw());

		if ((rc == SQLITE_ROW) || (rc == SQLITE_DONE)) {
			// Check if stmt contains any columns.
			Int colNum = sqlite3_column_count(stmt);
			if (colNum > 0)
				result = true;

			reset();
		} else {
			Str *msg = new (this) Str((wchar*)sqlite3_errmsg16(db->raw()));
			throw new (this) SQLError(msg);
		}
	}

	void SQLite_Statement::finalize() {
		if (stmt) {
			sqlite3_finalize(stmt);
			stmt = null;
			result = false;
		}
	}

	void SQLite_Statement::reset() {
		if (stmt)
			sqlite3_reset(stmt);
	}

	Row * SQLite_Statement::fetch() const {
		if (!result)
			return null; // Maybe return null?

		Engine &e = this -> engine();
		Array<Variant> * row = new (this) Array<Variant>;

		if (sqlite3_step(stmt) == SQLITE_DONE)
			return null;

		int num_column = sqlite3_column_count(stmt);
		for (int i = 0; i < num_column; i++){
			switch (sqlite3_column_type(stmt, i)) {
			case SQLITE3_TEXT:
				row->push(Variant(new (this)Str((wchar *)sqlite3_column_text16(stmt, i)), e));
				break;
			case SQLITE_INTEGER:
				row->push(Variant(sqlite3_column_int(stmt,i), e));
				break;
			case SQLITE_FLOAT:
				row->push(Variant(sqlite3_column_double(stmt,i), e));
				break;
			default:
				break;
			}
		}
		return new (this) Row(row);
	}

	Long SQLite_Statement::lastRowId() const {
		return lastId;
	}

	////////////////////////////////////
	//			   SQLite			  //
	////////////////////////////////////

	SQLite::~SQLite() {
		close();
	}

	SQLite::SQLite(Url * str) {
		int rc = sqlite3_open16(str -> format() -> c_str(), &db);

		if (rc)
			throw new (this) InternalError(TO_S(this, S("Can't open database: ") << rc));
	}

	SQLite::SQLite() {
		int rc = sqlite3_open(":memory:", &db);

		if (rc)
			throw new (this) InternalError(TO_S(this, S("Can't open database: ") << rc));
	}

	Statement * SQLite::prepare(Str *str) {
		return new (str) SQLite_Statement(this, str);
	}

	void SQLite::close() {
		sqlite3_close(db);
	}

	sqlite3 *SQLite::raw() const {
		return db;
	}

	Array<Str*>* SQLite::tables(){
		Str * str = new (this) Str(L"SELECT name FROM sqlite_master WHERE type IN ('table', 'view') AND name NOT LIKE 'sqlite%' ORDER BY 1");

		Array<Str*>* names = new (this) Array<Str*>;

		Statement * stmt = prepare(str);
		stmt->execute();

		Row * name;
		Statement::Iter i = stmt->iter();
		while (name = i.next())
			names->push(name->getStr(0));

		stmt->finalize();
		return names;
	}

	static bool isWS(wchar c) {
		switch (c) {
		case ' ':
		case '\n':
		case '\r':
		case '\t':
			return true;
		default:
			return false;
		}
	}

	static bool isSpecial(wchar c) {
		switch (c) {
		case '(':
		case ')':
		case ',':
		case '.':
		case ';':
			return true;
		default:
			return false;
		}
	}

	static const wchar *skipWS(const wchar *at) {
		for (; *at; at++) {
			if (!isWS(*at))
				break;
		}
		return at;
	}

	static const wchar *nextToken(const wchar *at) {
		if (*at == '"') {
			// Identifier. Quite easy, no escape sequences need to be taken into consideration (I think).
			for (at++; *at; at++) {
				if (*at == '"')
					return at + 1;
			}
		} else if (*at == '\'') {
			// String literal. '' is used as an "escape", otherwise easy.
			for (at++; *at; at++) {
				if (*at == '\'') {
					if (at[1] == '\'') {
						// Escaped, go on.
						at++;
					} else {
						// End. We're done.
						return at + 1;
					}
				}
			}
		} else if (isSpecial(*at)) {
			return at + 1;
		} else {
			// Some keyword or an unquoted literal.
			for (; *at; at++) {
				if (isWS(*at) || isSpecial(*at))
					return at;
			}
		}

		return at;
	}

	static bool cmp(const wchar *begin, const wchar *end, const wchar *str) {
		for (; begin != end; begin++, str++) {
			if (*str == 0)
				return false;
			if (*str != *begin)
				return false;
		}

		return *str == 0;
	}

	static Str *identifier(Engine &e, const wchar *begin, const wchar *end) {
		if (*begin == '"')
			return new (e) Str(begin + 1, end - 1);
		else
			return new (e) Str(begin, end);
	}

	static bool next(const wchar *&begin, const wchar *&end) {
		begin = skipWS(end);
		if (*begin == 0)
			return false;
		end = nextToken(begin);
		return true;
	}

	Schema *SQLite::schema(Str *table) {
		Str *query = new (this) Str(S("SELECT sql FROM sqlite_master WHERE type = 'table' and name = ?;"));
		Statement *prepared = prepare(query);
		prepared->bind(0, table);
		prepared->execute();
		Row *row = prepared->iter().next();
		if (!row)
			return null;

		const wchar *data = row->getStr(0)->c_str();
		const wchar *begin = data;
		const wchar *end = data;

		Str *tableName = null;
		{
			// Find the "(" and remember the name in the previous token.
			const wchar *nameBegin, *nameEnd;
			while (next(begin, end)) {
				if (cmp(begin, end, S("(")))
					break;

				nameBegin = begin;
				nameEnd = end;
			}

			tableName = identifier(engine(), nameBegin, nameEnd);
		}

		Array<Schema::Column *> *cols = new (this) Array<Schema::Column *>();
		while (next(begin, end)) {
			Str *colName = identifier(engine(), begin, end);
			next(begin, end);
			Str *typeName = identifier(engine(), begin, end);

			while (next(begin, end)) {
				if (cmp(begin, end, S(",")))
					break;

				// TODO: We need to handle these somehow... They are not easily separated without
				// implementing the entire SQL grammar more or less...
			}

			cols->push(new (this) Schema::Column(colName, typeName));
		}

		// TODO: Also look for indices (they are in sqlite_master as well).

		return new (this) Schema(tableName, cols);
	}

}
