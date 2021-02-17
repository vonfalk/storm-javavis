#pragma once
#include "Context.h"
#include "Session.h"
#include "Core/Net/NetStream.h"

namespace ssl {

	/**
	 * A context used to represent a server context.
	 *
	 * A server generally needs to provide a certificate of some sort to assert its identity.
	 */
	class ServerContext : public Context {
		STORM_CLASS;
	public:
		STORM_CTOR ServerContext();

	protected:
		// Create data.
		virtual SSLContext *createData();
	};

}
