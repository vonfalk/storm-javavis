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
		sqlite3_bind_text(stmt, pos, str->utf8_str(), -1, SQLITE_TRANSIENT);
	}

	void SQLite_Statement::bind(Nat pos, Int i) {
		sqlite3_bind_int(stmt, pos, (int)i);
	}
	void SQLite_Statement::bind(Nat pos, Double d) {
		sqlite3_bind_double(stmt, pos, (double)d);
	}

	void SQLite_Statement::execute() {
		Int rc = sqlite3_step(stmt);
		lastId = db->lastRowId();

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

	Long SQLite::lastRowId() const {
		Long i = 0;
		if (db != null) {
			i = (Long)sqlite3_last_insert_rowid(db);
		}
		return i;
	}

	void SQLite::close(){
		sqlite3_close(db);
	}

	sqlite3 * SQLite::raw() const {
		return db;
	}

	SQLite * STORM_FN SQLite::getDB() {
		return this;
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


	////////////////////////////////////
	//			   Schema			  //
	////////////////////////////////////

	Schema::Content::Content(Str* name, Str* dt, Array<Str*> *attributes) : name(name), dt(dt), attributes(attributes) {}

	Schema::Content::Content() : name(null), dt(null), attributes(null) {}

	Schema::Schema(Array<Content*> *row, Str *table_name) : row(row), table_name(table_name) {}

	Schema::Schema() : row(null), table_name(null) {}

	Schema * SQLite::schema(Str *str) {

		Str *query = new (this) Str(L"SELECT sql FROM sqlite_master WHERE type = 'table' and name = ?");

		const wchar *constraints[] = {
			S("DEFAULT"),
			S("UNIQUE"),
			S("CHECK"),
			S("PRIMARY KEY"),
			S("NOT NULL"),
			S("AUTOINCREMENT")
		};

		Schema * temp = new (this) Schema();

		Array<Schema::Content*>* row = new (this) Array<Schema::Content*>;
		Statement *stmt;
		stmt = prepare(query);
		stmt->bind(1, str);
		stmt->execute();

		Row * r;
		Statement::Iter i = stmt->iter();
		r = i.next();

		if (r == NULL)
			return new (this) Schema();

		Str *toManip = r->getStr(0);

		toManip = trimBlankLines(toManip);
		Str::Iter f, l;
		Int par = 0;
		Bool isForeignKey = false;

		// Find starting position
		for (Str::Iter it = toManip->begin(); it != toManip->end(); it++) {
			if (*it == '(') {
				f = it + 1;
				par = 1;
				break;
			}
		}
		if (par == 1) {
			for (Str::Iter it = f; it != toManip->end(); it++) {
				if (*it == '('){
					par++;
				}
				else if (*it == ')') {
					par--;
				}

				else if ((*it == ',') && ((*(it + 1) == '\n') || (*(it + 1) == '\r')) || (*(it + 1) == '\n') ) {
					l = it + 1;

					//rows
					Str * substr = trimBlankLines(removeIndentation(toManip->substr(f, l)));
					substr = temp->removeIndent(substr);

					Str * check = substr->substr(substr->begin(),substr->begin()+11);
					Str * fk = new (this) Str(L"FOREIGN KEY");

					if (*check == *fk) {
						isForeignKey = true;
					}

					Str * name = null;
					Str * dt = null;
					Array<Str*> * attributes = new (this) Array<Str*>();
					Int wspace = 0;
					Str::Iter first, last, frst;
					Bool done = false;

					first = substr->begin();
					for (Str::Iter iter = substr->begin(); iter != substr->end(); iter++) {
						//if foreign keys occur before last row
						if (isForeignKey) {
							l = it;
							Str * foreignKey = trimBlankLines(removeIndentation(toManip->substr(f, l)));

							foreignKey = temp->removeIndent(foreignKey);
							temp->getForeignKey(foreignKey, row);
							done = true;
						}

						if (done) {
							break;
						}

						if (((((iter + 1) == substr->end()) && ((*iter == ','))) || (*iter == ' ')) && ((wspace == 0) || (wspace == 1)) ) {

							if (wspace == 0) {
								last = iter;
								name  = substr->substr(first,last);
							}

							if (wspace == 1) {
								last = iter;
								dt = removeIndentation(substr->substr(first,last));
							}

							wspace++;
							first = iter;
						}

						if (iter + 1 == substr->end()) {

							last = iter;
							Str * temp = trimBlankLines(removeIndentation(substr->substr(first,last)));

							first = temp->begin();
							for (Str::Iter itt = temp->begin(); itt != temp->end(); itt++) {
								if (*itt == ' ' || (itt + 1) == temp->end() || ((*itt == ' ') && ((itt + 1) == temp->end())) ) {
									if (itt + 1 == temp->end() && !(*itt == ' '))
										last = itt + 1;
									else
										last = itt;

									Str *  match = trimBlankLines(removeIndentation(temp->substr(first,last)));

									for (nat i = 0;i < ARRAY_COUNT(constraints); i++) {
										if (*match == constraints[i]) {
											attributes->push(match);
											first = itt;
											break;
										}
									}
								}
							}
							wspace++;
						}

						if (wspace == 3) {
							Schema::Content * cont = new (this) Schema::Content(name,dt,attributes);

							row->push(cont);
							break;
						}

					}
					f = it + 1;

				}

				if (par == 0) {
					l = it;

					Str * lastrow = (trimBlankLines(removeIndentation(toManip->substr(f, l))));

					lastrow = temp->removeIndent(lastrow);
					temp->getForeignKey(lastrow, row);
				}
			}
		}
		stmt->finalize();
		Schema * s = new (this) Schema(row,str);

		return s;
	}

	Nat Schema::size() const  {
		if (row)
			return row->count();

		return 0;
	}

	Str * Schema::getTable() const {
		return table_name;
	}

	Schema::Content * Schema::getRow(Int idx) const {
		return row->at(idx);
	}

	void Schema::getForeignKey(Str * foreignKey, Array<Content*>* row) {

		Int i = 0;
		Str::Iter f, l;

		for (Str::Iter iter = foreignKey->begin(); iter != foreignKey->end(); iter++) {
			if (*iter == '[') {
				f = iter;
			}

			if (*iter == ']') {
				l = iter;
				Str * name = foreignKey->substr(f,l);
				Str * ch = new (this) Str(']');
				name = *name + ch;


				for (Array<Content*>::Iter it = row->begin(); it != row->end(); it++) {
					if (*row->at(i)->getName() == *name) {
						Array<Str*> * attr = row->at(i)->getAttr();

						attr->push(foreignKey);
						row->at(i)->setAttr(attr);
						break;
					}
					i++;
				}
				break;
			}
		}
	}


	Str * Schema::Content::getName() const {
		return name;
	}

	Array<Str*> * Schema::Content::getAttr() const {
		return attributes;
	}

	Str * Schema::Content::getDt() const {
		return dt;
	}

	void Schema::Content::setAttr(Array<Str*> * attr) {
		attributes = attr;
	}

	Str * Schema::removeIndent(Str * str) {
		Str::Iter last;
		last = str->end();
		for (Str::Iter iter = str->begin(); iter != str->end(); iter++) {
			if (!(*iter == ' ')) {
				str = str->substr(iter,last);
				break;
			}
		}
		return str;

	}

}
