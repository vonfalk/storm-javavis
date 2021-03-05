#include "stdafx.h"
#include "OpenSSLError.h"

#ifdef POSIX

#include "Exception.h"
#include "Core/Convert.h"
#include <openssl/x509.h>
#include <openssl/err.h>

namespace ssl {

	void checkError() {
		unsigned long error = ERR_get_error();
		if (error) {
			Engine &e = runtime::someEngine();
			char buffer[256]; // Is enough according to the docs.
			ERR_error_string(error, buffer);

			const wchar *desc = toWChar(e, buffer, 256)->v;
			throw new (e) SSLError(TO_S(e, S("Error from OpenSSL: ") << desc));
		}
	}

	void throwError() {
		unsigned long error = ERR_get_error();
		Engine &e = runtime::someEngine();
		char buffer[256]; // Is enough according to the docs.
		ERR_error_string(error, buffer);

		const wchar *desc = toWChar(e, buffer, 256)->v;
		throw new (e) SSLError(TO_S(e, S("Error from OpenSSL: ") << desc));
	}

	const wchar *certError(long code) {
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

}

#endif
