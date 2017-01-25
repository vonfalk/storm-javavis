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

			// Serialize a SExpr for sending. Handles null properly.
			void STORM_FN serialize(OStream *to, MAYBE(SExpr *) msg);

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
		};

	}
}
