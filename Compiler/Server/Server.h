#pragma once
#include "Compiler/Thread.h"
#include "Connection.h"

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

		private:
			// Connection.
			Connection *conn;

			// Keep track of some symbols we need.
			Symbol *quit;

			// Process a message.
			Bool process(SExpr *msg);

			// Convenience functions for printing things.
			void print(Str *s);
			void print(const wchar *s);
			void print(const CString &s);
		};

	}
}
