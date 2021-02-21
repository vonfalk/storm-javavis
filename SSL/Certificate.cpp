#include "stdafx.h"
#include "Certificate.h"
#include "WinCert.h"
#include "OpenSSLCert.h"
#include "Exception.h"
#include "Core/Io/Text.h"

namespace ssl {

	Certificate *Certificate::loadPEM(Str *data) {
		SSLCert *c = null;
#ifdef WINDOWS
		c = WinSSLCert::fromPEM(data);
#else
		c = OpenSSLCert::fromPEM(data);
#endif
		return new (data) Certificate(c);
	}

	Certificate *Certificate::loadPEM(Url *file) {
		return loadPEM(readAllText(file));
	}

	Certificate::Certificate(SSLCert *data) : data(data) {}

	Certificate::Certificate(const Certificate &o) : data(o.data) {
		if (data)
			data->ref();
	}

	Certificate::~Certificate() {
		if (data)
			data->unref();
		data = null;
	}

	void Certificate::toS(StrBuf *to) const {
		if (data)
			data->output(to);
		else
			*to << S("<no certificate>");
	}


	CertificateKey *Certificate::loadKeyPEM(Str *data) {
		SSLCertKey *c = null;
#ifdef WINDOWS
		c = WinSSLCertKey::fromPEM(data);
#else
		c = OpenSSLCertKey::fromPEM(data);
#endif
		return new (this) CertificateKey(this, c);
	}

	CertificateKey *Certificate::loadKeyPEM(Url *file) {
		return loadKeyPEM(readAllText(file));
	}


	CertificateKey::CertificateKey(Certificate *cert, SSLCertKey *data) : cert(cert), data(data) {
		const wchar *error = data->validate(cert->get());
		if (error) {
			delete data;
			data = null;
			throw new (this) SSLError(TO_S(this, S("The key is invalid for ") << cert << S(": ") << error));
		}
	}

	CertificateKey::CertificateKey(const CertificateKey &o) : cert(o.cert), data(o.data) {
		if (data)
			data->ref();
	}

	CertificateKey::~CertificateKey() {
		clear();
	}

	void CertificateKey::clear() {
		if (data)
			data->unref();
		data = null;
	}

	void CertificateKey::toS(StrBuf *to) const {
		if (data)
			*to << S("Key to certificate: ") << cert;
		else
			*to << S("<no certificate key>");
	}

}
