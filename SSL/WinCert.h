#pragma once
#include "Data.h"

#ifdef WINDOWS

#define SECURITY_WIN32
#include <Security.h>
#include <Schannel.h>

namespace ssl {

	/**
	 * Certificate implementation on Windows.
	 */
	class WinSSLCert : public SSLCert {
	public:
		// Load from PEM file.
		static WinSSLCert *fromPEM(Str *data);

		// The actual data.
		const CERT_CONTEXT *data;

		// Functions.
		WinSSLCert *windows() override;
		OpenSSLCert *openSSL() override;
		void output(StrBuf *to) override;

		// Destroy.
		~WinSSLCert();

	private:
		// Create.
		WinSSLCert(const CERT_CONTEXT *data);
	};

}

#endif
