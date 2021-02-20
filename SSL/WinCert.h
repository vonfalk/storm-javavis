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


	/**
	 * Certificate key implementation on Windows.
	 */
	class WinSSLCertKey : public SSLCertKey {
	public:
		~WinSSLCertKey();

		// Load from PEM file.
		static WinSSLCertKey *fromPEM(Str *data);

		// The actual data.
		HCRYPTPROV provider;
		HCRYPTKEY key;

		// Functions.
		bool validate(Engine &e, SSLCert *cert) override;

	private:
		// Create.
		WinSSLCertKey(HCRYPTPROV provider, HCRYPTKEY key);
	};

}

#endif
