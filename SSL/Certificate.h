#pragma once
#include "Data.h"
#include "Core/Io/Url.h"

namespace ssl {

	/**
	 * A X509 certificate.
	 *
	 * In its plain form, this class represents a certificate without a key. This can be used to
	 * verify that a server contains a particular certificate, etc.
	 *
	 * To create new connections as a server with a particular certificate, you need a certificate
	 * key as well.
	 */
	class Certificate : public Object {
		STORM_CLASS;
	public:
		// Load a certificate from a PEM string or file.
		static Certificate *STORM_FN loadPEM(Str *data);
		static Certificate *STORM_FN loadPEM(Url *file);

		// Copy.
		Certificate(const Certificate &o);

		// Destroy.
		~Certificate();

	protected:
		// To string.
		virtual void STORM_FN toS(StrBuf *to) const;

	private:
		// Create.
		Certificate(SSLCert *data);

		// The underlying data.
		SSLCert *data;
	};

}
