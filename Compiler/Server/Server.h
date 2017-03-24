#pragma once
#include "Compiler/Thread.h"
#include "Connection.h"
#include "File.h"
#include "Test.h"
#include "WorkQueue.h"
#include "RangeSet.h"

namespace storm {
	namespace server {
		STORM_PKG(core.lang.server);

		// Since the preprocessor does not know ::String, we make a typedef.
		typedef ::String CString;

		/**
		 * Class encapsulating all state in the server communication.
		 */
		class Server : public ObjectOn<Compiler> {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR Server(Connection *c);

			// Run the server loop. Blocks until the server is detached.
			void STORM_FN run();

			// Run delayed work.
			void runWork(WorkItem *item);

		private:
			// Connection.
			Connection *conn;

			// Open files.
			Map<Nat, File *> *files;

			// Keep track of some symbols we need.
			Symbol *quit;
			Symbol *open;
			Symbol *edit;
			Symbol *point;
			Symbol *close;
			Symbol *indent;
			Symbol *test;
			Symbol *debug;
			Symbol *recolor;
			Symbol *color;
			Symbol *level;
			Symbol *as;

			// Any test state required now?
			Test *testState;

			// Work queue.
			WorkQueue *work;

			// Keep track of the color symbols used.
			Array<Symbol *> *colorSyms;

			// Find the symbol to be used for a specific color.
			Symbol *colorSym(syntax::TokenColor color);

			// Process a message.
			Bool process(SExpr *msg);

			// Handle specific messages.
			void onOpen(SExpr *msg);
			void onEdit(SExpr *msg);
			void onPoint(SExpr *msg);
			void onClose(SExpr *msg);
			void onIndent(SExpr *msg);
			void onTest(SExpr *msg);
			void onDebug(SExpr *msg);
			void onReColor(SExpr *msg);

			// Send updates for 'range' in 'file'.
			void update(File *file, Range range);

			// Size of the chunks to send to the client.
			static const Nat chunkChars = 8000;

			// Send updates for 'range' in 'file', sending one reasonabley-sized chunk now and
			// scheduling the rest for later.
			void updateLater(File *file, Range range);

			// Convenience functions for printing things.
			void print(Str *s);
			void print(const wchar *s);
			void print(const CString &s);
		};


		/**
		 * Schedule file updates.
		 *
		 * Stores a number of ranges to be updated so that multiple updaters do not interfere and
		 * cause too much data to be sent on each idle period.
		 */
		class UpdateFileRange : public WorkItem {
			STORM_CLASS;
		public:
			STORM_CTOR UpdateFileRange(File *file);
			STORM_CTOR UpdateFileRange(File *file, Range range);

			// Add a range.
			void STORM_FN add(Range range);

			// Merge with another item.
			Bool STORM_FN merge(WorkItem *o);

			// Execute this item.
			virtual Range STORM_FN run(WorkQueue *q);

		private:
			// Set to update.
			RangeSet *update;
		};

	}
}
