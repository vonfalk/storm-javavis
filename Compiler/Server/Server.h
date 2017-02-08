#pragma once
#include "Compiler/Thread.h"
#include "Connection.h"
#include "File.h"
#include "Test.h"
#include "WorkQueue.h"

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
			Symbol *close;
			Symbol *test;
			Symbol *debug;
			Symbol *recolor;
			Symbol *color;

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
			void onClose(SExpr *msg);
			void onTest(SExpr *msg);
			void onDebug(SExpr *msg);
			void onReColor(SExpr *msg);

			// Send updates for 'range' in 'file'.
			void update(File *file, Range range);

			// Convenience functions for printing things.
			void print(Str *s);
			void print(const wchar *s);
			void print(const CString &s);
		};

	}
}
