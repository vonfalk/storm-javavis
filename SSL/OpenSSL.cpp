#include "stdafx.h"
#include "OpenSSL.h"

#ifdef POSIX

#include "Exception.h"
#include "Core/Convert.h"

#include <openssl/err.h>
#include <openssl/opensslv.h>
#include <openssl/opensslconf.h>
#ifndef OPENSSL_THREADS
#error "No thread support in OpenSSL!"
#endif
#if OPENSSL_VERSION_NUMBER < 0x10100000
#error "Too old OpenSSL version, we rely on some locking support!"
#endif

namespace ssl {

	/**
	 * Throw a suitable exception on SSL error.
	 */
	static void checkError() {
		unsigned long error = ERR_get_error();
		if (error) {
			Engine &e = runtime::someEngine();
			char buffer[256]; // Is enough according to the docs.
			ERR_error_string(error, buffer);

			const wchar *desc = toWChar(e, buffer, 256)->v;
			throw new (e) SSLError(TO_S(e, S("Error from OpenSSL: ") << desc));
		}
	}

	/**
	 * Initialze the library properly.
	 */
	class Initializer {
	public:
		Initializer() {
			SSL_library_init();
			SSL_load_error_strings();
			CONF_modules_load_file(NULL, NULL, 0);

			// From the headers of OpenSSL, it seems like the requirements of locking functions was
			// removed earlier. As such, we don't need to give them anymore.
		}
	};

	void init() {
		static Initializer i;
	}

	static int verifyCallback(int preverifyOk, X509_STORE_CTX *x509) {
		PLN(L"TODO: Don't blindly trust the certificates!");
		return 1;

		// See the manpage for SSL_CTX_set_verify for an example of how to get access to some context in here.
		// 0 - fail immediately
		// 1 - continue
		if (preverifyOk != 1)
			return 0;
		return 1;
	}

	OpenSSLContext *OpenSSLContext::createClient() {
		init();
		// Note: This means SSLv3 or TLSv1.x, as available.
		OpenSSLContext *ctx = new OpenSSLContext(SSL_CTX_new(TLS_client_method()));

		SSL_CTX_set_verify(ctx->context, SSL_VERIFY_PEER, verifyCallback);
		SSL_CTX_set_verify_depth(ctx->context, 4); // Why 4?
		// Make configurable.
		long flags = SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1;
		SSL_CTX_set_options(ctx->context, flags);

		// Use default paths for certificates (TODO: make configurable).
		SSL_CTX_set_default_verify_paths(ctx->context);

		// Allowed ciphers. We should probably modify this list a bit...
		SSL_CTX_set_cipher_list(ctx->context, "HIGH:!aNULL:!kRSA:!PSK:!SRP:!MD5:!RC4");

		return ctx;
	}

	OpenSSLContext *OpenSSLContext::createServer() {
		init();
		return new OpenSSLContext(SSL_CTX_new(TLS_server_method()));
	}

	OpenSSLContext::OpenSSLContext(SSL_CTX *ctx) : context(ctx) {
		if (!ctx)
			throw new (runtime::someEngine()) SSLError(S("Failed to create OpenSSL context."));
	}

	OpenSSLContext::~OpenSSLContext() {
		SSL_CTX_free(context);
	}

	SSLSession *OpenSSLContext::createSession() {
		return new OpenSSLSession(this);
	}

	/**
	 * Data structure containing GC pointers that we allocate for our BIO class.
	 *
	 * TODO: Eventually, we might want to keep this somewhere else.
	 */
	struct BIO_data {
		IStream *input;
		OStream *output;
	};

	// Pointer offsets in BIO_data
	static const size_t BIO_data_offsets[] = {
		OFFSET_OF(BIO_data, input),
		OFFSET_OF(BIO_data, output),
	};

	// GCtype for the data.
	class BioType {
	public:
		BioType() {
			type = (GcType *)malloc(gcTypeSize(ARRAY_COUNT(BIO_data_offsets)));
			type->kind = GcType::tFixed;
			type->type = null;
			type->finalizer = null;
			type->stride = sizeof(BIO_data);
			type->count = ARRAY_COUNT(BIO_data_offsets);
			for (size_t i = 0; i < ARRAY_COUNT(BIO_data_offsets); i++)
				type->offset[i] = BIO_data_offsets[i];
		}
		~BioType() {
			free(type);
		}

		GcType *type;
	};

	// Allocate BIO data for streams. Ensures that this data is not moved by the GC. It still needs
	// to be kept alive somehow.
	static BIO_data *allocData(IStream *input, OStream *output) {
		static BioType t;
		BIO_data *data = (BIO_data *)runtime::allocStaticRaw(input->engine(), t.type);
		data->input = input;
		data->output = output;
		return data;
	}

	/**
	 * A BIO wrapper that forwards to/from our streams.
	 */

	// Maximum number of bytes to read/write in one go.
	static int bioWrite(BIO *bio, const char *src, int length) {
		BIO_data *data = (BIO_data *)BIO_get_data(bio);
		GcPreArray<Byte, 4096> buffer;

		size_t offset = 0;
		while (offset < size_t(length)) {
			size_t toWrite = min(buffer.count, size_t(length) - offset);
			memcpy(buffer.v, src + offset, toWrite);

			Buffer b = fullBuffer(buffer);
			b.filled(toWrite);
			data->output->write(b);

			offset += toWrite;
		}

		return length;
	}

	static int bioRead(BIO *bio, char *dest, int length) {
		BIO_data *data = (BIO_data *)BIO_get_data(bio);

		// Note: Will ensure so that 'size' is not too large.
		GcPreArray<Byte, 4096> buffer(length);

		// Note: This implementation might give to little data back, but that is fine.
		Buffer b = emptyBuffer(buffer);
		b = data->input->read(b);
		memcpy(dest, b.dataPtr(), b.filled());

		return int(b.filled());
	}

	static int bioPuts(BIO *bio, const char *buffer) {
		return bioWrite(bio, buffer, strlen(buffer));
	}

	static int bioGets(BIO *bio, char *buffer, int size) {
		// TODO: Implement this... It should work like BIO_puts()
		// I don't think it is strictly necessary though. The network BIOs do not
		// support gets, so we're probably fine.
		PLN(L"Call to bioGets");
		return 0;
	}

	static long int bioCtrl(BIO *bio, int cmd, long larg, void *parg) {
		BIO_data *data = (BIO_data *)BIO_get_data(bio);

		switch (cmd) {
		case BIO_CTRL_RESET:
			PLN("BIO_CTRL_RESET");
			break;
		case BIO_CTRL_EOF:
			PLN("BIO_CTRL_EOF");
			break;
		case BIO_CTRL_SET_CLOSE:
			PLN("BIO_CTRL_SET_CLOSE");
			break;
		case BIO_CTRL_GET_CLOSE:
			PLN("BIO_CTRL_GET_CLOSE");
			break;
		case BIO_CTRL_PENDING:
			PLN("BIO_CTRL_PENDING");
			break;
		case BIO_CTRL_WPENDING:
			PLN("BIO_CTRL_WPENDING");
			break;
		case BIO_CTRL_FLUSH:
			data->output->flush();
			return 1;
		case BIO_CTRL_GET_CALLBACK:
			PLN("BIO_CTRL_GET_CALLBACK");
			break;
		case BIO_CTRL_SET_CALLBACK:
			PLN("BIO_CTRL_SET_CALLBACK");
			break;
		}
		return 0;
	}

	static long int bioCtrlCb(BIO *bio, int cmd, BIO_info_cb *cb) {
		PVAR(cmd);
		return 0;
	}

	class StormBioMethod {
	public:
		StormBioMethod() {
			int type = BIO_get_new_index();
			method = BIO_meth_new(BIO_TYPE_SOURCE_SINK | type, "STORM I/O");

			// We could call BIO_meth_free() at shutdown...

			// Note: There are *_ex versions of read and write. Maybe we should use them instead?
			BIO_meth_set_write(method, bioWrite);
			BIO_meth_set_read(method, bioRead);
			BIO_meth_set_puts(method, bioPuts);
			BIO_meth_set_gets(method, bioGets);
			BIO_meth_set_ctrl(method, bioCtrl);
			BIO_meth_set_callback_ctrl(method, bioCtrlCb);
			// There are create and destroy as well, but I don't think we need them.
		}

		~StormBioMethod() {
			BIO_meth_free(method);
		}

		BIO_METHOD *method;
	};

	// Create a BIO that wraps a Storm istream and an ostream.
	// Assumes that "data" is kept alive by storing a reference to it somewhere else.
	static BIO *BIO_new_storm(BIO_data *data) {
		static StormBioMethod m;
		BIO *bio = BIO_new(m.method);
		BIO_set_data(bio, data);
		return bio;
	}

	// We can chain BIOs using:
	// con = BIO_new(stormBio)
	// ssl = BIO_new_ssl(ctx, 1) // 1 is for clients
	// result = BIO_push(ssl, con)

	OpenSSLSession::OpenSSLSession(OpenSSLContext *ctx) : context(ctx) {
		context->ref();
	}

	OpenSSLSession::~OpenSSLSession() {
		context->unref();
	}

	static const wchar *certError(long code) {
		switch (code) {
		case X509_V_ERR_UNSPECIFIED:
			return S("Unspecified error; should not happen.");
		case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT:
			return S("The issuer certificate of a looked up certificate could not be found. This normally means the list of trusted certificates is not complete.");
		case X509_V_ERR_UNABLE_TO_GET_CRL:
			return S("The CRL of a certificate could not be found.");
		case X509_V_ERR_UNABLE_TO_DECRYPT_CERT_SIGNATURE:
			return S("The certificate signature could not be decrypted. This means that the actual signature value could not be determined rather than it not matching the expected value, this is only meaningful for RSA keys.");
		case X509_V_ERR_UNABLE_TO_DECRYPT_CRL_SIGNATURE:
			return S("The CRL signature could not be decrypted: this means that the actual signature value could not be determined rather than it not matching the expected value. Unused.");
		case X509_V_ERR_UNABLE_TO_DECODE_ISSUER_PUBLIC_KEY:
			return S("The public key in the certificate SubjectPublicKeyInfo could not be read.");
		case X509_V_ERR_CERT_SIGNATURE_FAILURE:
			return S("The signature of the certificate is invalid.");
		case X509_V_ERR_CRL_SIGNATURE_FAILURE:
			return S("The signature of the certificate is invalid.");
		case X509_V_ERR_CERT_NOT_YET_VALID:
			return S("The certificate is not yet valid: the notBefore date is after the current time.");
		case X509_V_ERR_CERT_HAS_EXPIRED:
			return S("The certificate has expired: that is the notAfter date is before the current time.");
		case X509_V_ERR_CRL_NOT_YET_VALID:
			return S("The CRL is not yet valid.");
		case X509_V_ERR_CRL_HAS_EXPIRED:
			return S("The CRL has expired.");
		case X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD:
			return S("The certificate notBefore field contains an invalid time.");
		case X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD:
			return S("The certificate notAfter field contains an invalid time.");
		case X509_V_ERR_ERROR_IN_CRL_LAST_UPDATE_FIELD:
			return S("The CRL lastUpdate field contains an invalid time.");
		case X509_V_ERR_ERROR_IN_CRL_NEXT_UPDATE_FIELD:
			return S("The CRL nextUpdate field contains an invalid time.");
		case X509_V_ERR_OUT_OF_MEM:
			return S("An error occurred trying to allocate memory. This should never happen.");
		case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT:
			return S("The passed certificate is self-signed and the same certificate cannot be found in the list of trusted certificates.");
		case X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN:
			return S("The certificate chain could be built up using the untrusted certificates but the root could not be found locally.");
		case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY:
			return S("The issuer certificate could not be found: this occurs if the issuer certificate of an untrusted certificate cannot be found.");
		case X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE:
			return S("No signatures could be verified because the chain contains only one certificate and it is not self signed.");
		case X509_V_ERR_CERT_CHAIN_TOO_LONG:
			return S("The certificate chain length is greater than the supplied maximum depth. Unused.");
		case X509_V_ERR_CERT_REVOKED:
			return S("The certificate has been revoked.");
		case X509_V_ERR_INVALID_CA:
			return S("A CA certificate is invalid. Either it is not a CA or its extensions are not consistent with the supplied purpose.");
		case X509_V_ERR_PATH_LENGTH_EXCEEDED:
			return S("The basicConstraints pathlength parameter has been exceeded.");
		case X509_V_ERR_INVALID_PURPOSE:
			return S("The supplied certificate cannot be used for the specified purpose.");
		case X509_V_ERR_CERT_UNTRUSTED:
			return S("The root CA is not marked as trusted for the specified purpose.");
		case X509_V_ERR_CERT_REJECTED:
			return S("The root CA is marked to reject the specified purpose.");
		case X509_V_ERR_SUBJECT_ISSUER_MISMATCH:
			return S("Not used as of OpenSSL 1.1.0 as a result of the deprecation of the -issuer_checks option.");
		case X509_V_ERR_AKID_SKID_MISMATCH:
			return S("Not used as of OpenSSL 1.1.0 as a result of the deprecation of the -issuer_checks option.");
		case X509_V_ERR_AKID_ISSUER_SERIAL_MISMATCH:
			return S("Not used as of OpenSSL 1.1.0 as a result of the deprecation of the -issuer_checks option.");
		case X509_V_ERR_KEYUSAGE_NO_CERTSIGN:
			return S("Not used as of OpenSSL 1.1.0 as a result of the deprecation of the -issuer_checks option.");
		case X509_V_ERR_UNABLE_TO_GET_CRL_ISSUER:
			return S("Unable to get CRL issuer certificate.");
		case X509_V_ERR_UNHANDLED_CRITICAL_EXTENSION:
			return S("Unhandled critical extension.");
		case X509_V_ERR_KEYUSAGE_NO_CRL_SIGN:
			return S("Key usage does not include CRL signing.");
		case X509_V_ERR_UNHANDLED_CRITICAL_CRL_EXTENSION:
			return S("Unhandled critical CRL extension.");
		case X509_V_ERR_INVALID_NON_CA:
			return S("Invalid non-CA certificate has CA markings.");
		case X509_V_ERR_PROXY_PATH_LENGTH_EXCEEDED:
			return S("Proxy path length constraint exceeded.");
		case X509_V_ERR_KEYUSAGE_NO_DIGITAL_SIGNATURE:
			return S("Key usage does not include digital signature.");
		case X509_V_ERR_PROXY_CERTIFICATES_NOT_ALLOWED:
			return S("Proxy certificates not allowed, please use -allow_proxy_certs.");
		case X509_V_ERR_INVALID_EXTENSION:
			return S("Invalid or inconsistent certificate extension.");
		case X509_V_ERR_INVALID_POLICY_EXTENSION:
			return S("Invalid or inconsistent certificate policy extension.");
		case X509_V_ERR_NO_EXPLICIT_POLICY:
			return S("No explicit policy.");
		case X509_V_ERR_DIFFERENT_CRL_SCOPE:
			return S("Different CRL scope.");
		case X509_V_ERR_UNSUPPORTED_EXTENSION_FEATURE:
			return S("Unsupported extension feature.");
		case X509_V_ERR_UNNESTED_RESOURCE:
			return S("RFC 3779 resource not subset of parent's resources.");
		case X509_V_ERR_PERMITTED_VIOLATION:
			return S("Permitted subtree violation.");
		case X509_V_ERR_EXCLUDED_VIOLATION:
			return S("Excluded subtree violation.");
		case X509_V_ERR_SUBTREE_MINMAX:
			return S("Name constraints minimum and maximum not supported.");
		case X509_V_ERR_APPLICATION_VERIFICATION:
			return S("Application verification failure. Unused.");
		case X509_V_ERR_UNSUPPORTED_CONSTRAINT_TYPE:
			return S("Unsupported name constraint type.");
		case X509_V_ERR_UNSUPPORTED_CONSTRAINT_SYNTAX:
			return S("Unsupported or invalid name constraint syntax.");
		case X509_V_ERR_UNSUPPORTED_NAME_SYNTAX:
			return S("Unsupported or invalid name syntax.");
		case X509_V_ERR_CRL_PATH_VALIDATION_ERROR:
			return S("CRL path validation error.");
		case X509_V_ERR_PATH_LOOP:
			return S("Path loop.");
		case X509_V_ERR_SUITE_B_INVALID_VERSION:
			return S("Suite B: certificate version invalid.");
		case X509_V_ERR_SUITE_B_INVALID_ALGORITHM:
			return S("Suite B: invalid public key algorithm.");
		case X509_V_ERR_SUITE_B_INVALID_CURVE:
			return S("Suite B: invalid ECC curve.");
		case X509_V_ERR_SUITE_B_INVALID_SIGNATURE_ALGORITHM:
			return S("Suite B: invalid signature algorithm.");
		case X509_V_ERR_SUITE_B_LOS_NOT_ALLOWED:
			return S("Suite B: curve not allowed for this LOS.");
		case X509_V_ERR_SUITE_B_CANNOT_SIGN_P_384_WITH_P_256:
			return S("Suite B: cannot sign P-384 with P-256.");
		case X509_V_ERR_HOSTNAME_MISMATCH:
			return S("Hostname mismatch.");
		case X509_V_ERR_EMAIL_MISMATCH:
			return S("Email address mismatch.");
		case X509_V_ERR_IP_ADDRESS_MISMATCH:
			return S("IP address mismatch.");
		case X509_V_ERR_DANE_NO_MATCH:
			return S("DANE TLSA authentication is enabled, but no TLSA records matched the certificate chain.  This error is only possible in s_client(1).");
		case X509_V_ERR_EE_KEY_TOO_SMALL:
			return S("EE certificate key too weak.");
		case X509_V_ERR_INVALID_CALL:
			return S("nvalid certificate verification context.");
		case X509_V_ERR_STORE_LOOKUP:
			return S("Issuer certificate lookup error.");
		case X509_V_ERR_NO_VALID_SCTS:
			return S("Certificate Transparency required, but no valid SCTs found.");
		case X509_V_ERR_PROXY_SUBJECT_NAME_VIOLATION:
			return S("Proxy subject name violation.");
		case X509_V_ERR_OCSP_VERIFY_NEEDED:
			return S("Returned by the verify callback to indicate an OCSP verification is needed.");
		case X509_V_ERR_OCSP_VERIFY_FAILED:
			return S("Returned by the verify callback to indicate OCSP verification failed.");
		case X509_V_ERR_OCSP_CERT_UNKNOWN:
			return S("Returned by the verify callback to indicate that the certificate is not recognized by the OCSP responder.");
		}
		return null;
	}

	void *OpenSSLSession::connect(IStream *input, OStream *output, Str *host) {
		BIO_data *data = allocData(input, output);
		BIO *io = BIO_new_storm(data);
		BIO *sslBio = BIO_new_ssl(context->context, 1); // 1 means "client".
		// According to what OpenSSL is doing, we don't need to free "io" manually. The docs are a
		// bit unclear about that though (at least the "BIO_push" manpage).
		connection = BIO_push(sslBio, io);

		SSL *ssl = null;
		BIO_get_ssl(connection, &ssl);
		SSL_set_tlsext_host_name(ssl, host->utf8_str());
		checkError();

		// Do the handshake!
		if (BIO_do_handshake(connection) != 1) {
			checkError();
		}

		X509 *cert = SSL_get_peer_certificate(ssl);
		if (cert) {
			X509_free(cert);
		} else {
			throw new (input) SSLError(S("The remote host did not present a certificate!"));
		}

		long certOk = SSL_get_verify_result(ssl);
		if (certOk == X509_V_OK) {
			PLN(L"Certificate OK.");
		} else {
			PLN(L"Certificate not ok: " << certOk);
			PVAR(certError(certOk));
		}

		// TODO: Hostname verification?
		// Example states that we should do it, but it does not say how.

		return data;
	}

	Bool SChannelSession::more(void *) {
		TODO(L"Fix me!");
		return true;
	}

	void SChannelSession::read(Buffer &to, void *) {
		os::Lock::L z(lock);

		int bytes = BIO_read(connection, to.dataPtr() + to.filled(), to.free());
		if (bytes > 0) {
			to.filled(to.filled() + bytes);
		} else if (bytes == 0) {
			// We are at EOF!
		} else {
			// Error.
		}
		TODO(L"Finish me!");
	}

	void SChannelSession::peek(Buffer &to, void *) {
		os::Lock::L z(lock);

		TODO(L"Finish me!");
	}

	void SChannelSession::write(const Buffer &from, Nat offset, void *) {
		os::Lock::L z(lock);

		BIO_write(connection, from.dataPtr() + offset, from.filled() - offset);
	}

	void SChannelSession::flush(void *gcData) {
		os::Lock::L z(lock);

		BIO_flush(connection);
	}

	void SChannelSession::shutdown(void *gcData) {
		os::Lock::L z(lock);

		TODO(L"Implement shutdown properly!");
	}

	void SChannelSession::close(void *gcData) {
		os::Lock::L z(lock);

		// TODO: We could do this through the BIO interface.
		BIO_data *data = (BIO_data *)gcData;
		data->src->close();
		data->dst->close();
	}

}

#endif
