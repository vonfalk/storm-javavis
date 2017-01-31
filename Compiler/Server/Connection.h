#pragma once
#include "SExpr.h"
#include "Core/Str.h"
#include "Core/Map.h"
#include "Core/Io/Stream.h"
#include "Core/Io/MemStream.h"
#include "Core/Io/Text.h"
#include "BufStream.h"

namespace storm {
	namespace server {
		STORM_PKG(core.lang.server);

		/**
		 * Connection enpoint to a client.
		 */
		class Connection : public Object {
			STORM_CLASS;
		public:
			// Create. Use the streams for communication.
			STORM_CTOR Connection(IStream *input, OStream *output);

			// Create a symbol.
			Symbol *symbol(const wchar *sym);
			Symbol *STORM_FN symbol(Str *sym);
			MAYBE(Symbol *) STORM_FN symbol(Nat id);

			// Shall symbol 'x' be sent? Only call during serialization, otherwise we will get out of sync!
			Bool STORM_FN sendSymbol(Symbol *sym);

			// Output used for stdout.
			TextOutput *textOut;

			// Send a message.
			void STORM_FN send(SExpr *msg);

			// Wait for a message. Any 'null' messages are ignored. If 'null' is returned, we received EOF.
			MAYBE(SExpr *) STORM_FN receive();

			/**
			 * Low-level read/write interface.
			 */

			// Write a SExpr to a stream. Handles null properly.
			void STORM_FN write(OStream *to, MAYBE(SExpr *) msg);

		private:
			// Remember created symbols.
			typedef Map<Str *, Symbol *> NameMap;
			Map<Str *, Symbol *> *symNames;

			// Remember which id:s are assigned to which symbols. Only symbols which have been sent
			// are in here.
			typedef Map<Nat, Symbol *> IdMap;
			Map<Nat, Symbol *> *symIds;

			// Last allocated symbol ID (we start from the high end).
			Nat lastSymId;

			// Input and output streams.
			IStream *input;
			OStream *output;

			// Data having been read from 'input' but not yet processed.
			BufStream *inputBuffer;

			// Read and process one message from the input stream. A 'null' message means that we
			// either received 'nil or a string which was passed on to standard input. 'false' is
			// returned if there is not enough data in the stream to process another message.
			Bool read(SExpr *&result);

			// Fill the input buffer with new data from 'input'. Return 'false' on error.
			Bool fillBuffer();

			// Read a SExpr from a stream.
			MAYBE(SExpr *) readSExpr(IStream *from, Bool &ok);

			// Read a partial SExpression given we know its header.
			MAYBE(SExpr *) readCons(IStream *from, Bool &ok);
			MAYBE(SExpr *) readNumber(IStream *from, Bool &ok);
			MAYBE(SExpr *) readString(IStream *from, Bool &ok);
			MAYBE(SExpr *) readNewSymbol(IStream *from, Bool &ok);
			MAYBE(SExpr *) readOldSymbol(IStream *from, Bool &ok);
		};

	}
}
