#pragma once
#include "Data.h"
#include "Core/Io/Url.h"

namespace ssl {

	class CertificateKey;

	/**
	 * A X509 certificate.
	 *
	 * To create new connections as a server with a particular certificate, you need a certificate
	 * key as well, as represented by a CertificateKey class.
	 *
	 * Acts much like a class even though it is a value.
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

		// Get the data.
		inline SSLCert *get() const { return data; }

		// Load a key for this certificate.
		CertificateKey *STORM_FN loadKeyPEM(Str *data);
		CertificateKey *STORM_FN loadKeyPEM(Url *file);

	protected:
		// To string.
		virtual void STORM_FN toS(StrBuf *to) const;

	private:
		// Create.
		Certificate(SSLCert *data);

		// The underlying data.
		UNKNOWN(PTR_NOGC) SSLCert *data;
	};


	/**
	 * A private key for a X509 certificate.
	 *
	 * Load from a certificate.
	 */
	class CertificateKey : public Object {
		STORM_CLASS;

		friend class Certificate;
	public:
		// Copy.
		CertificateKey(const CertificateKey &o);

		// Destroy.
		~CertificateKey();

		// Flush the key from memory immediately, if you don't want to wait for a GC.
		void STORM_FN clear();

		// Get the certificate.
		inline Certificate *certificate() const { return cert; }

		// Get the data.
		inline SSLCertKey *get() const { return data; }

	protected:
		// To string.
		virtual void STORM_FN toS(StrBuf *to) const;

	private:
		// Create.
		CertificateKey(Certificate *cert, SSLCertKey *data);

		// The certificate we're associated with.
		Certificate *cert;

		// The underlying data.
		UNKNOWN(PTR_NOGC) SSLCertKey *data;
	};

}
