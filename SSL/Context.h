#pragma once
#include "Data.h"
#include "Core/Io/Stream.h"
#include "Core/Net/Socket.h"

namespace ssl {

	/**
	 * An SSL context.
	 *
	 * Represents all settings on how the SSL connections are to be made. For example, what ciphers
	 * to trust, what certificates to trust, etc.
	 *
	 * This is the base class for common functionality. You probably want to create either
	 * ServerContext or ClientContext in order to create Endpoints.
	 */
	class Context : public Object {
		STORM_ABSTRACT_CLASS;
	public:
		// Create.
		Context();

		// Copy.
		Context(const Context &o);

		// Destroy.
		virtual ~Context();

		// Deep copy.
		virtual void STORM_FN deepCopy(CloneEnv *env);

		// Only use strong ciphers. Disables cipher suites with known weaknesses, but compatibility may suffer.
		Bool STORM_FN strongCiphers() const { return onlyStrong; }
		void STORM_ASSIGN strongCiphers(Bool b);

	protected:
		// Creates 'data' as needed.
		SSLContext *data();

		// Called whenever some state changes, so that we need to re-create data at some point.
		void invalidate();

		// Called whenever a new instance of data needs to be created.
		virtual SSLContext *createData();

	private:
		// The context is designed to keep a set of settings inside of itself and then create any
		// objects needed by the backend on demand. This pointer is ref-counted so that it is
		// constant once we created it. We also share it between copies of this object.
		SSLContext *created;

		// Only strong ciphers.
		Bool onlyStrong;
	};

}
