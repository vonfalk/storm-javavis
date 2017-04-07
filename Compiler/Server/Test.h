#pragma once
#include "Compiler/Thread.h"
#include "Connection.h"

namespace storm {
	namespace server {
		STORM_PKG(core.lang.server);

		/**
		 * Handles all state and communication related to the 'test'-messages.
		 */
		class Test : public ObjectOn<Compiler> {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR Test(Connection *c);

			// Handle a test message. Returns a reply to be sent or null.
			MAYBE(SExpr *) STORM_FN onMessage(SExpr *expr);

		private:
			// Keep track of some symbols we use.
			Symbol *start;
			Symbol *stop;
			Symbol *sum;
			Symbol *send;
			Symbol *echo;

			// Send this sum when 'stop' is called.
			Nat sumResult;

			// Connection.
			Connection *conn;

			// Clear all state.
			void clear();

			// Handle messages.
			MAYBE(SExpr *) STORM_FN onStop(SExpr *msg);
			MAYBE(SExpr *) STORM_FN onSum(SExpr *msg);
			MAYBE(SExpr *) STORM_FN onSend(SExpr *msg);
			MAYBE(SExpr *) STORM_FN onEcho(SExpr *msg);
		};

	}
}
