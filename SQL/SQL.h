#pragma once
#include "Core/Array.h"
#include "Core/Variant.h"
#include "Schema.h"

namespace sql {

    /**
     * This is representing the result of a Row from a SQL SELECT statement query.
     */
    class Row : public Object {
        STORM_CLASS;
    public:
        STORM_CTOR Row();
        STORM_CTOR Row(Array<Variant> * v);

        //If column is of type Str, use this to get the result. (TEXT or VARCHAR in sqlite3)
        Str* STORM_FN getStr(Int idx);

        //If column is of type Int, use this to get the result. (INTEGER in sqlite3)
        Int * STORM_FN getInt(Int idx);

        //If column is of type Double, use this to get the result. (REAL in sqlite3)
        Double* STORM_FN getDouble(Int idx);
    private:
        Array<Variant> * v;
    };

    /**
     * A pure virtual Statement class.
	 *
     * This class specifies which functions have to be made if you're creating your own SQL language in Storm.
     */
    class Statement : public Object {
        STORM_ABSTRACT_CLASS;
    public:
        Statement();

		// Execute this prepared statement. Throws on error.
        virtual void STORM_FN execute() ABSTRACT;

		// Bind parameters.
        virtual void STORM_FN bind(Int pos, Str *str) ABSTRACT;
        virtual void STORM_FN bind(Int pos, Int i) ABSTRACT;
        virtual void STORM_FN bind(Int pos, Double d) ABSTRACT;
        virtual void STORM_FN finalize() ABSTRACT;
        virtual void STORM_FN reset() ABSTRACT;

		// Fetch a row.
        virtual MAYBE(Row *) STORM_FN fetch() const ABSTRACT;

		// Get the last row id.
        virtual Long STORM_FN lastRowId() const ABSTRACT;

        class Iter {
            STORM_VALUE;
        public:
            Iter();
            Iter(const Statement *stmt);

            MAYBE(Row *) STORM_FN next();
        private:
            const Statement *owner;
        };

		// Iterator interface.
        Iter STORM_FN iter();
    protected:
        Long lastId;
    };


    /**
	 * A pure virtual Database class.
	 *
	 * This class specifies which functions have to be made when inheriting from Database if you're
	 * creating your own SQL language in Storm.
	 */
	class Database : public Object {
        STORM_ABSTRACT_CLASS;
    public:
        virtual Statement * STORM_FN prepare(Str * str) ABSTRACT;
        virtual Long STORM_FN lastRowId() const ABSTRACT;
        virtual void STORM_FN close() ABSTRACT;
        virtual Array<Str*>* STORM_FN tables() ABSTRACT;
        virtual Schema * STORM_FN schema(Str * str) ABSTRACT;
    };


    /**
	 * Empty database implementation.
	 *
	 * Useful for dealing with edge-cases without having to use maybe-types. Throws an exception
	 * when any member is invoked.
	 */
	class EmptyDB : public Database {
        STORM_CLASS;
    public:
        STORM_CTOR EmptyDB();

        Statement * STORM_FN prepare(Str *str) override;
        Long STORM_FN lastRowId() const override;
        void STORM_FN close() override;
        Array<Str*>* STORM_FN tables() override;
        Schema * STORM_FN schema(Str * str) override;
    };
}
