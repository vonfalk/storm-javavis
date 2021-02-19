#include "stdafx.h"
#include "SecureChannel.h"
#include "Exception.h"
#include "SecureChannelData.h"
#include "WinErrorMsg.h"

#ifdef WINDOWS

namespace ssl {

	// Inspiration for this implementation:
	// Racket: https://github.com/racket/racket/blob/master/racket/collects/net/win32-ssl.rkt
	// MSDN: https://docs.microsoft.com/en-us/windows/win32/secauthn/creating-a-secure-connection-using-schannel

	// Note: We could use InitSecurityInterface and use the function table returned from there. I'm
	// not sure what is the main benefit of that though.

	SChannelContext::SChannelContext(CredHandle creds) : credentials(creds) {}

	SChannelContext::~SChannelContext() {
		FreeCredentialsHandle(&credentials);
	}

	SChannelContext *SChannelContext::createClient() {
		SECURITY_STATUS status;
		CredHandle credentials;

		SCHANNEL_CRED data = {
			SCHANNEL_CRED_VERSION,
			0, // Number of private keys
			NULL, // Array of private keys
			NULL, // Root store
			0, // Reserved
			0, // Reserved
			0, // Number of algorithms to use (0 = system default)
			NULL, // Algorithms to use (0 = system default)
			0, // Protocols (we should set it to zero to let the system decide). This is where we specify TLS version.
			0, // Cipher strenght. 0 = system default
			0, // Session lifespan. 0 = default (10 hours)
			0, // Various flags. We might want to alter this later.
			0, // Kernel format. Should be zero.
		};

		// Flags to consider:
		// SCH_CRED_NO_DEFAULT_CREDS - don't try to find a certificate chain.
		// SCH_CRED_NO_SERVERNAME_CHECK - don't check the server name.
		// SCH_CRED_USE_DEFAULT_CREDS - probably good as a default? Seems to be the default
		// SCH_USE_STRONG_CRYPTO - disable suites with known weaknesses.

		// Acquire credentials.
		status = AcquireCredentialsHandle(NULL, SCHANNEL_NAME, SECPKG_CRED_OUTBOUND,
										NULL, &data, NULL, NULL,
										&credentials, NULL);

		if (status == SEC_E_SECPKG_NOT_FOUND)
			throw new (runtime::someEngine()) SSLError(S("Failed to find the SChannel security package."));
		else if (status != SEC_E_OK)
			throwError(runtime::someEngine(), S("Failed to acquire credentials for SChannel: "), status);

		return new SChannelContext(credentials);
	}

	SChannelContext *SChannelContext::createServer() {
		SECURITY_STATUS status;
		CredHandle credentials;

		// TODO: Keys!
		// According to the documentation, keys should be in "array of private keys", not "root
		// store".  The root store is used to verify clients.

		SCHANNEL_CRED data = {
			SCHANNEL_CRED_VERSION,
			0, // Number of private keys
			NULL, // Array of private keys
			NULL, // Root store
			0, // Reserved
			0, // Reserved
			0, // Number of algorithms to use (0 = system default)
			NULL, // Algorithms to use (0 = system default)
			0, // Protocols (we should set it to zero to let the system decide). This is where we specify TLS version.
			0, // Cipher strenght. 0 = system default
			0, // Session lifespan. 0 = default (10 hours)
			0, // Various flags.
			0, // Kernel format. Should be zero.
		};

		// TODO: Add flags.

		status = AcquireCredentialsHandle(NULL, SCHANNEL_NAME, SECPKG_CRED_INBOUND,
										NULL, &data, NULL, NULL,
										&credentials, NULL);

		if (status == SEC_E_SECPKG_NOT_FOUND)
			throw new (runtime::someEngine()) SSLError(S("Failed to find the SChannel security package."));
		else if (status != SEC_E_OK)
			throwError(runtime::someEngine(), S("Failed to acquire credentials for SChannel: "), status);

		return new SChannelContext(credentials);
	}

	SSLSession *SChannelContext::createSession() {
		return new SChannelSession(this);
	}


	SChannelSession::SChannelSession(SChannelContext *data)
		: data(data), maxMessageSize(0), blockSize(0) {

		data->ref();
		SecInvalidateHandle(&context);
	}

	SChannelSession::~SChannelSession() {
		data->unref();
		if (SecIsValidHandle(&context))
			DeleteSecurityContext(&context);
	}

	int SChannelSession::initSession(Engine &e, Buffer &inBuffer, Buffer &outBuffer, const wchar *host) {
		SECURITY_STATUS status;
		// Ask it to allocate output buffers for us. That means we don't have to worry about the size of them.
		ULONG requirements = ISC_REQ_ALLOCATE_MEMORY;
		// Some more good features.
		requirements |= ISC_REQ_REPLAY_DETECT | ISC_REQ_SEQUENCE_DETECT | ISC_REQ_CONFIDENTIALITY | ISC_REQ_STREAM;
		ULONG attrsOut = 0;
		CtxtHandle *currentHandle = null;
		SecBufferDesc *inputParam = null;

		SecBuffer inBuffers[2] = {
			{ 0, SECBUFFER_TOKEN, null },
			{ 0, SECBUFFER_EMPTY, null }
		};
		SecBufferDesc input = {
			SECBUFFER_VERSION, 2, inBuffers
		};

		if (!SecIsValidHandle(&context)) {
			// First call.
			currentHandle = null;
			inputParam = null;
		} else {
			// Other calls.
			currentHandle = &context;
			inputParam = &input;
			inBuffers[0].cbBuffer = inBuffer.filled();
			inBuffers[0].pvBuffer = inBuffer.dataPtr();
		}

		// Note: Doc says we need a SECBUFFER_ALERT, but it works anyway.
		SecBuffer outBuffers[1] = { { 0, SECBUFFER_EMPTY, null } };
		SecBufferDesc output = {
			SECBUFFER_VERSION, 1, outBuffers,
		};

		status = InitializeSecurityContext(&data->credentials, currentHandle,
										(SEC_WCHAR *)host, requirements,
										0 /* reserved */, 0 /* not used for schannel */,
										inputParam, 0 /* reserved */,
										&context, &output,
										&attrsOut, NULL /* expires */);

		// See if there is any unconsumed data in the input buffer.
		if (input.pBuffers[1].BufferType == SECBUFFER_EXTRA) {
			// Shift data back and keep unconsumed data in the beginning of "input".
			inBuffer.shift(inBuffer.filled() - input.pBuffers[1].cbBuffer);
		} else if (status == SEC_E_INCOMPLETE_MESSAGE) {
			// It was incomplete, just don't clear the buffer.
		} else {
			// No unconsumed data. Clear it.
			inBuffer.filled(0);
		}

		int result = 0;
		bool error = false;
		if (status == SEC_I_CONTINUE_NEEDED) {
			// Send to server!
			SecBuffer b = output.pBuffers[0];
			if (outBuffer.count() < b.cbBuffer)
				outBuffer = storm::buffer(e, b.cbBuffer);

			memcpy(outBuffer.dataPtr(), b.pvBuffer, b.cbBuffer);
			outBuffer.filled(b.cbBuffer);

			// Not done. More messages.
			result = -1;
		} else if (status == SEC_E_OK) {
			// All done, set the maximum size for encryption/decryption.
			initSizes();
			result = 0;
		} else if (status == SEC_E_INCOMPLETE_MESSAGE) {
			if (input.pBuffers[1].BufferType == SECBUFFER_MISSING) {
				result = input.pBuffers[1].cbBuffer;
			} else {
				// Just provide a decent guess if we cant find a guess from the API.
				result = 32;
			}
		} else {
			error = true;
		}

		// Clean up.
		if (output.pBuffers[0].pvBuffer)
			FreeContextBuffer(output.pBuffers[0].pvBuffer);

		if (error)
			throwError(e, S("Failed to initialize an SSL session: "), status);

		return result;
	}

	void SChannelSession::encrypt(Engine &e, const Buffer &input, Nat offset, Nat inputSize, Buffer &output) {
		// "reserve" size for header and trailer.
		Nat size = headerSize + trailerSize + inputSize;
		if (output.count() < size)
			output = buffer(e, size);

		output.filled(size);
		memcpy(output.dataPtr() + headerSize, input.dataPtr() + offset, inputSize);

		SecBuffer buffers[4] = {
			{ headerSize, SECBUFFER_STREAM_HEADER, output.dataPtr() },
			{ inputSize, SECBUFFER_DATA, output.dataPtr() + headerSize },
			{ trailerSize, SECBUFFER_STREAM_TRAILER, output.dataPtr() + headerSize + inputSize },
			{ 0, SECBUFFER_EMPTY, null },
		};
		SecBufferDesc bufferDesc = {
			SECBUFFER_VERSION, ARRAY_COUNT(buffers), buffers
		};
		SECURITY_STATUS status = EncryptMessage(&context, 0, &bufferDesc, 0);
		if (status != SEC_E_OK)
			throwError(e, S("Failed to encrypt a message: "), status);
	}

	static SecBuffer *findBuffer(SecBufferDesc &in, unsigned long type) {
		for (unsigned long i = 0; i < in.cBuffers; i++) {
			if (in.pBuffers[i].BufferType == type)
				return &in.pBuffers[i];
		}
		return null;
	}

	SChannelSession::Markers SChannelSession::decrypt(Engine &e, Buffer &data, Nat offset) {
		SecBuffer buffers[4] = {
			{ data.filled() - offset, SECBUFFER_DATA, data.dataPtr() + offset },
			{ 0, SECBUFFER_EMPTY, null },
			{ 0, SECBUFFER_EMPTY, null },
			{ 0, SECBUFFER_EMPTY, null },
		};
		SecBufferDesc bufferDesc = {
			SECBUFFER_VERSION, ARRAY_COUNT(buffers), buffers
		};
		SECURITY_STATUS status = DecryptMessage(&context, &bufferDesc, 0, NULL);

		Markers m = { offset, offset, offset, false };

		if (status == SEC_E_OK) {
			{
				SecBuffer *plaintext = findBuffer(bufferDesc, SECBUFFER_DATA);
				m.plaintextStart = (byte *)plaintext->pvBuffer - data.dataPtr();
				m.plaintextEnd = m.plaintextStart + plaintext->cbBuffer;
			}

			if (SecBuffer *ciphertext = findBuffer(bufferDesc, SECBUFFER_EXTRA)) {
				m.remainingStart = (byte *)ciphertext->pvBuffer - data.dataPtr();
			} else {
				m.remainingStart = data.filled();
			}
		} else if (status == SEC_E_INCOMPLETE_MESSAGE) {
			// Tell the caller that they need to get more data.
			// We don't need to do anything here due to the initialization above.
		} else if (status == SEC_I_CONTEXT_EXPIRED) {
			// The remote end shut the connection down.
			m.shutdown = true;
		} else {
			throwError(e, S("Failed to decrypt a message: "), status);
		}

		return m;
	}

	Buffer SChannelSession::shutdown(Engine &e) {
		{
			DWORD token = SCHANNEL_SHUTDOWN;
			SecBuffer buffers[1] = {
				{ sizeof(DWORD), SECBUFFER_TOKEN, &token }
			};
			SecBufferDesc bufferDesc = {
				SECBUFFER_VERSION, ARRAY_COUNT(buffers), buffers
			};
			SECURITY_STATUS status = ApplyControlToken(&context, &bufferDesc);
			if (status != SEC_E_OK)
				throwError(e, S("Failed to shutdown: "), status);
		}

		// Now, we just need to call initSession(?) to get what to reply!
		// TODO: We should call initSession for server/client as appropriate.
		// TODO: Doc says we might need to loop this. Based on the example
		// from http://www.coastrd.com/c-schannel-smtp, it seems unlikely for SChannel.
		Buffer input;
		Buffer output;
		int ok = initSession(e, input, output, null);
		if (ok != 0)
			WARNING(L"We might need to do something differently!");
		return output;
	}

	void SChannelSession::initSizes() {
		SecPkgContext_StreamSizes ss;
		QueryContextAttributes(&context, SECPKG_ATTR_STREAM_SIZES, &ss);
		maxMessageSize = ss.cbMaximumMessage;
		blockSize = ss.cbBlockSize;
		headerSize = ss.cbHeader;
		trailerSize = ss.cbTrailer;
	}

	/**
	 * The "public" API:
	 */

	void *SChannelSession::connect(IStream *input, OStream *output, Str *host) {
		Engine &e = input->engine();
		SChannelData *data = new (e) SChannelData(input, output);

		// Minimum "free" when calling receive.
		Nat minFree = 128;

		// Input and output buffers.
		Buffer inBuffer = storm::buffer(e, 2*1024);
		Buffer outBuffer = storm::buffer(e, 2*1024);

		while (true) {
			int result = initSession(e, inBuffer, outBuffer, host->c_str());

			if (result == 0) {
				break;
			} else if (result < 0) {
				// Send to server and receive more.
				if (outBuffer.filled() > 0) {
					data->dst->write(outBuffer);
					data->dst->flush();
				}
			}

			// Read more data.
			Nat more = max(minFree, Nat(abs(result)));
			if (inBuffer.free() < more)
				inBuffer = grow(e, inBuffer, inBuffer.filled() + more);

			inBuffer = data->src->read(inBuffer);
		}

		// From here on, we can use EncryptMessage and DecryptMessage.
		// We must use cbMaximumMessage from QueryContextAttributes to find max message size.

		// Resize the input and output buffers so that we don't have to resize them in the
		// future. We can't reallocate them and keep inter-thread consistency.
		data->bufferSizes = min(Nat(4*1024), maxMessageSize);
		data->incoming = storm::buffer(e, data->bufferSizes);

		return data;
	}

	Bool SChannelSession::more(void *gcData) {
		SChannelData *data = (SChannelData *)gcData;
		os::Lock::L z(lock);

		// More decrypted data?
		if (data->decryptedStart < data->decryptedEnd)
			return true;

		// If the remote end is shutdown, we won't get more data.
		if (data->incomingEnd)
			return false;

		// Otherwise, see if we have more data to decrypt, or if there is more in the source.
		return (data->remainingStart < data->incoming.filled()) // more encrypted data?
			|| data->src->more(); // more from the source stream?

		return true;
	}

	void SChannelSession::read(Buffer &to, void *gcData) {
		SChannelData *data = (SChannelData *)gcData;
		os::Lock::L z(lock);

		if (data->decryptedStart >= data->decryptedEnd)
			fill(data);

		Nat toFill = min(to.free(), data->decryptedEnd - data->decryptedStart);
		memcpy(to.dataPtr() + to.filled(), data->incoming.dataPtr() + data->decryptedStart, toFill);
		to.filled(to.filled() + toFill);
		data->decryptedStart += toFill;
	}

	void SChannelSession::peek(Buffer &to, void *gcData) {
		// TODO: More generous peeking, perhaps?

		SChannelData *data = (SChannelData *)gcData;
		os::Lock::L z(lock);

		if (data->decryptedStart >= data->decryptedEnd)
			fill(data);

		Nat toFill = min(to.free(), data->decryptedEnd - data->decryptedStart);
		memcpy(to.dataPtr() + to.filled(), data->incoming.dataPtr() + data->decryptedStart, toFill);
		to.filled(to.filled() + toFill);
	}

	void SChannelSession::write(const Buffer &from, Nat offset, void *gcData) {
		SChannelData *data = (SChannelData *)gcData;
		os::Lock::L z(lock);

		if (data->outgoingEnd)
			return;

		// Don't send more data than the max message size.
		// For SChannel, this seems to be 64k, so it is unlikely, but we shall handle it properly.
		while (offset < from.filled()) {
			Nat chunkSz = min(from.filled() - offset, maxMessageSize);
			encrypt(data->engine(), from, offset, chunkSz, data->outgoing);
			data->dst->write(data->outgoing);

			offset += chunkSz;
		}

		// Clear the buffer if it becomes too large.
		if (data->outgoing.count() > data->bufferSizes * 2)
			data->outgoing = Buffer();
	}

	void SChannelSession::flush(void *gcData) {
		SChannelData *data = (SChannelData *)gcData;
		os::Lock::L z(lock);
		data->dst->flush();
	}

	void SChannelSession::shutdown(void *gcData) {
		SChannelData *data = (SChannelData *)gcData;
		os::Lock::L z(lock);

		if (data->outgoingEnd)
			return;

		Buffer msg = shutdown(data->engine());
		data->dst->write(msg);

		data->outgoingEnd = true;
	}

	void SChannelSession::close(void *gcData) {
		SChannelData *data = (SChannelData *)gcData;
		os::Lock::L z(lock);

		data->src->close();
		data->dst->close();
	}

	void SChannelSession::fill(SChannelData *data) {
		// In case we fail somewhere...
		data->decryptedStart = data->decryptedEnd = 0;

		// Anything left in the buffer?
		if (data->remainingStart >= data->incoming.filled()) {
			// Nope. Get more. But this means that the buffer is entirely empty, use that.
			data->incoming.filled(0);
			data->remainingStart = 0;
			data->incoming = data->src->read(data->incoming);
		}

		// Should we shrink the buffer a bit?
		Nat used = data->incoming.filled() - data->remainingStart;
		if ((used * 2 < data->incoming.count()) && (data->incoming.count() > data->bufferSizes)) {
			Buffer t = buffer(data->engine(), max(used, data->bufferSizes));
			memcpy(t.dataPtr(), data->incoming.dataPtr() + data->remainingStart, used);
			data->incoming = t;
			data->incoming.filled(used);
			data->remainingStart = 0;
		}

		while (true) {
			SChannelSession::Markers markers = decrypt(data->engine(), data->incoming, data->remainingStart);
			data->decryptedStart = markers.plaintextStart;
			data->decryptedEnd = markers.plaintextEnd;
			data->remainingStart = markers.remainingStart;

			// Did we decrypt something? Good, we're done for now.
			if (data->decryptedStart != data->decryptedEnd)
				break;

			// End of transmission?
			if (markers.shutdown) {
				data->incomingEnd = true;
				break;
			}

			// Is there a large "hole" in the beginning of the array? If so, we want to move it back
			// into place to use the space more efficiently.
			if (data->remainingStart > data->incoming.count() / 4) {
				data->incoming.shift(data->remainingStart);
				data->remainingStart = 0;
				data->decryptedStart = data->decryptedEnd = 0;
			}

			// If we didn't get any data back, try to get more data and try again.
			Nat margin = data->incoming.filled() / 2;
			if (data->incoming.free() < margin)
				data->incoming = grow(data->engine(), data->incoming, data->incoming.filled() * 2);

			Nat before = data->incoming.filled();
			data->incoming = data->src->read(data->incoming);

			if (data->incoming.filled() == before) {
				// Failed to receive data, give up.
				break;
			}
		}
	}

}

#endif
