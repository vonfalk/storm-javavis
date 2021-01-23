#include "stdafx.h"
#include "OpenSSL.h"

#ifdef POSIX

#include "Exception.h"

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
		// See the manpage for SSL_CTX_set_verify for an example of how to get access to some context in here.
		// 0 - fail immediately
		// 1 - continue
		PLN(L"In verification!");
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

	/**
	 * Data structure containing GC pointers that we allocate for our BIO class.
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
		// I don't think it is strictly necessary though.
		PLN(L"Call to bioGets");
		return 0;
	}

	static long int bioCtrl(BIO *bio, int cmd, long larg, void *parg) {
		// TODO: Flush might be interesting here.
		return 0;
	}

	static long int bioCtrlCb(BIO *bio, int cmd, BIO_info_cb *cb) {
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

}

#endif
