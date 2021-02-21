#pragma once
#include "Data.h"

#ifdef POSIX

#include <openssl/evp.h>
#include <openssl/x509.h>

namespace ssl {

	/**
	 * Certificate implementation for OpenSSL.
	 */
	class OpenSSLCert : public SSLCert {
	public:
		// Load from PEM file.
		static OpenSSLCert *fromPEM(Str *data);

		// Actual data.
		X509 *data;

		// Functions.
		WinSSLCert *windows() override;
		OpenSSLCert *openSSL() override;
		void output(StrBuf *to) override;

		// Destroy.
		~OpenSSLCert();

	private:
		// Create.
		OpenSSLCert(X509 *data);
	};

	/**
	 * Certificate key implementation for OpenSSL.
	 */
	class OpenSSLCertKey : public SSLCertKey {
	public:
		~OpenSSLCertKey();

		// Load from PEM file.
		static OpenSSLCertKey *fromPEM(Str *data);

		// Key.
		EVP_PKEY *data;

		// Functions.
		const wchar *validate(SSLCert *cert) override;
		WinSSLCertKey *windows() override;
		OpenSSLCertKey *openSSL() override;

	private:
		// Create.
		OpenSSLCertKey(EVP_PKEY *data);
	};

}

#endif
