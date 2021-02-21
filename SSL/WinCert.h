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

		// Make a copy of this certificate.
		WinSSLCert *copy();

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

		// The actual data. Decoded and ready to be imported.
		std::vector<byte> data;

		// Functions.
		bool validate(Engine &e, SSLCert *cert) override;
		WinSSLCertKey *windows() override;
		OpenSSLCertKey *openSSL() override;

	private:
		// Create.
		WinSSLCertKey(const std::vector<byte> &data);
	};


	/**
	 * Helper class for setting up a temporary HCRYPTPROV storing a particular key so that we can
	 * pass it on to SChannel.
	 *
	 * The destructor makes sure that this (potentially named) container is released.
	 *
	 * Note: Since this class may represent a named (globally in the system) entity in the system,
	 * it is essential that it is kept alive for as short as possible. If we crash, our mess will
	 * remain in the system, and will be difficult to find and clean up.
	 */
	class WinKeyStore {
	public:
		// Create a WinKeyStore with the specified name containing the specified key.
		// If 'anonymous' is true, we create a non-persistent store.
		WinKeyStore(WinSSLCertKey *key, bool anonymous);

		// Destroy.
		~WinKeyStore();

		// The crypto provider, if needed.
		HCRYPTPROV provider;

		// The key stored inside the provider, if needed.
		HCRYPTKEY key;

		// The name of the provider.
		wchar name[50];

		// Attach a certificate to the key stored here.
		void attach(PCCERT_CONTEXT certificate);

	private:
		WinKeyStore(const WinKeyStore &o);
		WinKeyStore &operator =(const WinKeyStore &o);
	};

}

#endif
