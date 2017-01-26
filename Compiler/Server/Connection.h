#pragma once
#include "SExpr.h"
#include "Core/Str.h"
#include "Core/Map.h"
#include "Core/Io/Stream.h"
#include "Core/Io/Text.h"

namespace storm {
	namespace server {
		STORM_PKG(core.lang.server);

		/**
		 * De-serialization result.
		 */
		class ReadResult {
			STORM_VALUE;
		public:
			STORM_CTOR ReadResult();
			STORM_CTOR ReadResult(Nat consumed, MAYBE(SExpr *) result);

			// # of bytes consumed. 0 = failed.
			Nat consumed;

			// Resulting SExpr.
			MAYBE(SExpr *) result;

			// Failed?
			inline Bool STORM_FN failed() const {
				return consumed == 0;
			}

			// Increase number of bytes consumed.
			inline ReadResult STORM_FN operator +(Nat bytes) const {
				return ReadResult(consumed + bytes, result);
			}
		};


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
			TextWriter *textOut;

			// Send a message.
			void STORM_FN send(SExpr *msg);

			// Wait for a message (the message may be 'null').
			MAYBE(SExpr *) STORM_FN receive();

			/**
			 * Low-level read/write interface.
			 */

			// Write a SExpr to a stream. Handles null properly.
			void STORM_FN write(OStream *to, MAYBE(SExpr *) msg);

			// Read a SExpr from a stream.
			ReadResult STORM_FN read(IStream *from);

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

			// Read a partial SExpression given we know its header.
			ReadResult readCons(IStream *from);
			ReadResult readNumber(IStream *from);
			ReadResult readString(IStream *from);
			ReadResult readNewSymbol(IStream *from);
			ReadResult readOldSymbol(IStream *from);
		};

	}
}
