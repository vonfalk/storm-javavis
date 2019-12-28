SQL
====

The package `sql` contains an experimental generic database interface. It currently only supports
SQLite databases, but allows additional database connections to be added in the future.

The SQL extension was created by: David Ångström, Markus Larsson, Mohammed Hamade, Oscar Falk, Robin
Gustavsson, Saima Akhter

The package also contains a language extension to Basic Storm that allows SQL statements to be
executed directly from Basic Storm. For example:

```
void fn(Database db) {
    Str param = "Test";
    INSERT INTO table(id, name) VALUES (1, param) DB db;
}
```

It is also possible to declare table layouts in Storm and associate a database connection with them,
which makes it possible for Storm to type-check the SQL queries before they are executed (the
type-checking is not yet implemented, but the work needed to support it is done):

```
DATABASE Test {
    TABLE Test (
        A INTEGER PRIMARY KEY,
	B TEXT NOT NULL
    );
}

void fn(Databse db) {
    Test connection;
    // This makes sure that the database contains the table Test with the proper columns.
    connection.connect(db);

    var result = SELECT A, B FROM Test WHERE A > 10 DB connection;
    for (row in result) {
        print(row.getInt(0).toS);
    }
}
```

As can be seen from the previous example, the `SELECT` statement returns a Storm object. Currently,
it is necessary to extract the columns manually, but with the supplied type information it is
possible to produce proper types so that the following is possible:

```
void fn() {
    // ...
    var result = SELECT A, B FROM Test WHERE A > 10 DB connection;
    for (row in result) {
        print(row.A.toS);
    }
}
```

For more information, see the README-file in the package by typing `help sql` in the Basic Storm
interactive prompt.
