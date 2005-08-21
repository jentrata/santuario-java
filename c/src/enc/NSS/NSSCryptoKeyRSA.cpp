/*
 * Copyright 2002-2004 The Apache Software Foundation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * imitations under the License.
 */

/*
 * XSEC
 *
 * NSSCryptoKeyRSA := RSA Keys
 *
 * Author(s): Milan Tomic
 *
 * $Id$
 *
 */

#include <xsec/enc/NSS/NSSCryptoProvider.hpp>
#include <xsec/enc/NSS/NSSCryptoKeyRSA.hpp>
#include <xsec/enc/XSCrypt/XSCryptCryptoBase64.hpp>
#include <xsec/enc/XSECCryptoException.hpp>
#include <xsec/framework/XSECError.hpp>

#include <xercesc/util/Janitor.hpp>

XSEC_USING_XERCES(ArrayJanitor);

#if !defined (CRYPT_OAEP)
#	define CRYPT_OAEP              0x00000040
#	define KP_OAEP_PARAMS          36
#endif

NSSCryptoKeyRSA::NSSCryptoKeyRSA(HCRYPTPROV prov) {

	// Create a new key to be loaded as we go

	m_key = 0;
	m_p = prov;
	m_keySpec = 0;

	mp_exponent = NULL;
	m_exponentLen = 0;
	mp_modulus = NULL;
	m_modulusLen = 0;

};

NSSCryptoKeyRSA::~NSSCryptoKeyRSA() {


	// If we have a RSA, delete it

	if (m_key == 0)
		CryptDestroyKey(m_key);

	if (mp_exponent)
		delete[] mp_exponent;
	if (mp_modulus)
		delete[] mp_modulus;

};

NSSCryptoKeyRSA::NSSCryptoKeyRSA(HCRYPTPROV prov, 
										 HCRYPTKEY k) :
m_p(prov) {

	m_key = k;		// NOTE - We OWN this handle
	m_keySpec = 0;

	mp_exponent = mp_modulus = NULL;
	m_exponentLen = m_modulusLen = 0;

}

NSSCryptoKeyRSA::NSSCryptoKeyRSA(HCRYPTPROV prov, 
										 DWORD keySpec,
										 bool isPrivate) :
m_p(prov) {

	if (isPrivate == false) {

		throw XSECCryptoException(XSECCryptoException::RSAError,
			"NSSCryptoKeyRSA - Public keys defined via keySpec ctor not yet supported");

		
	}

	if (!CryptGetUserKey(prov, keySpec, &m_key)) {
		throw XSECCryptoException(XSECCryptoException::RSAError,
			"NSS:RSA Unable to retrieve user key");
	}

	//m_key = 0;
	m_keySpec = keySpec;

	mp_exponent = mp_modulus = NULL;
	m_exponentLen = m_modulusLen = 0;

}

// Generic key functions

XSECCryptoKey::KeyType NSSCryptoKeyRSA::getKeyType() {

	// Find out what we have
	if (m_key == 0) {

		if (m_keySpec != 0)
			return KEY_RSA_PRIVATE;

		if (mp_exponent == NULL ||
			mp_modulus == NULL)
			return KEY_NONE;
		else
			return KEY_RSA_PUBLIC;

	}

	if (m_keySpec != 0)
		return KEY_RSA_PAIR;

	return KEY_RSA_PUBLIC;

}

// --------------------------------------------------------------------------------
//           OAEP parameters handling
// --------------------------------------------------------------------------------


void NSSCryptoKeyRSA::setOAEPparams(unsigned char * params, unsigned int paramsLen) {

	if (params != NULL && paramsLen != 0)
		throw XSECCryptoException(XSECCryptoException::UnsupportedError,
			"NSS::setOAEPParams - OAEP parameters are not supported by Windows Crypto API");

}

unsigned int NSSCryptoKeyRSA::getOAEPparamsLen(void) {

	return 0;

}

const unsigned char * NSSCryptoKeyRSA::getOAEPparams(void) {

	return NULL;

}


// --------------------------------------------------------------------------------
//           Load key from parameters
// --------------------------------------------------------------------------------

void NSSCryptoKeyRSA::loadPublicModulusBase64BigNums(const char * b64, unsigned int len) {

	if (mp_modulus != NULL) {
		delete[] mp_modulus;
		mp_modulus = NULL;		// In case we get an exception
	}

	//mp_modulus = NSSCryptoProvider::b642WinBN(b64, len, m_modulusLen);

}

void NSSCryptoKeyRSA::loadPublicExponentBase64BigNums(const char * b64, unsigned int len) {

	if (mp_exponent != NULL) {
		delete[] mp_exponent;
		mp_exponent = NULL;		// In case we get an exception
	}

	//mp_exponent = NSSCryptoProvider::b642WinBN(b64, len, m_exponentLen);

}

void NSSCryptoKeyRSA::importKey(void) {
	
	if (m_key != 0 ||
		mp_exponent == NULL ||
		mp_modulus == NULL)

		return;


	// Create a RSA Public-Key blob

	// First build a buffer to hold everything

	BYTE * blobBuffer;
	unsigned int blobBufferLen = NSS_BLOBHEADERLEN + NSS_RSAPUBKEYLEN + m_modulusLen;
	XSECnew(blobBuffer, BYTE[blobBufferLen]);
	ArrayJanitor<BYTE> j_blobBuffer(blobBuffer);

	// Blob header
	BLOBHEADER * header = (BLOBHEADER *) blobBuffer;

	header->bType = PUBLICKEYBLOB;
	header->bVersion = 0x02;			// We are using a version 2 blob
	header->reserved = 0;
	header->aiKeyAlg = CALG_RSA_SIGN;

	// Now the public key header
	RSAPUBKEY * pubkey = (RSAPUBKEY *) (blobBuffer + NSS_BLOBHEADERLEN);

	pubkey->magic = 0x31415352;			// ASCII encoding of RSA1
	pubkey->bitlen = m_modulusLen * 8;		// Number of bits in prime modulus
	pubkey->pubexp = 0;
	BYTE * i = ((BYTE *) &(pubkey->pubexp));
	for (unsigned int j = 0; j < m_exponentLen; ++j)
		*i++ = mp_exponent[j];

	// Now copy in the modulus
	i = (BYTE *) (pubkey);
	i += NSS_RSAPUBKEYLEN;

	memcpy(i, mp_modulus, m_modulusLen);

	// Now that we have the blob, import
	BOOL fResult = CryptImportKey(
					m_p,
					blobBuffer,
					blobBufferLen,
					0,				// Not signed
					0,				// No flags
					&m_key);

	if (fResult == 0) {
		
		throw XSECCryptoException(XSECCryptoException::RSAError,
			"NSS:RSA Error attempting to import key parameters");

	}

}


// --------------------------------------------------------------------------------
//           Verify a signature encoded as a Base64 string
// --------------------------------------------------------------------------------

bool NSSCryptoKeyRSA::verifySHA1PKCS1Base64Signature(const unsigned char * hashBuf, 
								 unsigned int hashLen,
								 const char * base64Signature,
								 unsigned int sigLen) {

	// Use the currently loaded key to validate the Base64 encoded signature

	if (m_key == 0) {

		// Try to import from the parameters
		importKey();

		if (m_key == 0) {
			throw XSECCryptoException(XSECCryptoException::RSAError,
				"NSS:RSA - Attempt to validate signature with empty key");
		}
	}

	// Decode the signature
	unsigned char * rawSig;
	DWORD rawSigLen;
	XSECnew(rawSig, BYTE [sigLen]);
	ArrayJanitor<BYTE> j_rawSig(rawSig);

	// Decode the signature
	XSCryptCryptoBase64 b64;

	b64.decodeInit();
	rawSigLen = b64.decode((unsigned char *) base64Signature, sigLen, rawSig, sigLen);
	rawSigLen += b64.decodeFinish(&rawSig[rawSigLen], sigLen - rawSigLen);
	
	BYTE * rawSigFinal;
	XSECnew(rawSigFinal, BYTE[rawSigLen]);
	ArrayJanitor<BYTE> j_rawSigFinal(rawSigFinal);

	BYTE * j, *l;
	j = rawSig;
	l = rawSigFinal + rawSigLen - 1;
	
	while (l >= rawSigFinal) {
		*l-- = *j++;
	}

	// Have to create a Windows hash object and feed in the hash
	BOOL fResult;
	HCRYPTHASH h;
	fResult = CryptCreateHash(m_p, 
					CALG_SHA1, 
					0, 
					0,
					&h);

	if (!fResult) {
		throw XSECCryptoException(XSECCryptoException::RSAError,
			"NSS:RSA - Error creating Windows Hash Object");
	}

	// Feed the hash value into the newly created hash object
	fResult = CryptSetHashParam(
					h, 
					HP_HASHVAL,
					(unsigned char *) hashBuf,
					0);

	if (!fResult) {
		throw XSECCryptoException(XSECCryptoException::RSAError,
			"NSS:RSA - Error Setting Hash Value in Windows Hash object");
	}

	// Now validate
	fResult = CryptVerifySignature(
				h,
				rawSigFinal,
				rawSigLen,
				m_key,
				NULL,
				0);

	if (!fResult) {

		DWORD error = GetLastError();

		if (error != NTE_BAD_SIGNATURE) {
			throw XSECCryptoException(XSECCryptoException::RSAError,
			"NSS:RSA - Error occurred in RSA validation");
		}

		return false;
	}

	return true;

}

// --------------------------------------------------------------------------------
//           Sign and encode result as a Base64 string
// --------------------------------------------------------------------------------


unsigned int NSSCryptoKeyRSA::signSHA1PKCS1Base64Signature(unsigned char * hashBuf,
		unsigned int hashLen,
		char * base64SignatureBuf,
		unsigned int base64SignatureBufLen) {

	// Sign a pre-calculated hash using this key

	if (m_keySpec == 0) {

		throw XSECCryptoException(XSECCryptoException::RSAError,
			"NSS:RSA - Attempt to sign data using a public or un-loaded key");
	}

	// Have to create a Windows hash object and feed in the hash
	BOOL fResult;
	HCRYPTHASH h;
	fResult = CryptCreateHash(m_p, 
					CALG_SHA1, 
					0, 
					0,
					&h);

	if (!fResult) {
		throw XSECCryptoException(XSECCryptoException::RSAError,
			"NSS:RSA - Error creating Windows Hash Object");
	}

	// Feed the hash value into the newly created hash object
	fResult = CryptSetHashParam(
					h, 
					HP_HASHVAL, 
					hashBuf, 
					0);

	if (!fResult) {
		throw XSECCryptoException(XSECCryptoException::RSAError,
			"NSS:RSA - Error Setting Hash Value in Windows Hash object");
	}

	// Now sign
	DWORD rawSigLen;
	fResult = CryptSignHash(
				h,
				m_keySpec,
				NULL,
				0,
				NULL,
				&rawSigLen);

	if (!fResult || rawSigLen < 1) {

		throw XSECCryptoException(XSECCryptoException::RSAError,
		"NSS:RSA - Error occurred obtaining RSA sig length");
	}

	BYTE * rawSig;
	XSECnew(rawSig, BYTE[rawSigLen]);
	ArrayJanitor<BYTE> j_rawSig(rawSig);

	fResult = CryptSignHash(
				h,
				m_keySpec,
				NULL,
				0,
				rawSig,
				&rawSigLen);

	if (!fResult || rawSigLen < 1) {

		throw XSECCryptoException(XSECCryptoException::RSAError,
		"NSS:RSA - Error occurred signing hash");
	}

	// Now encode into a signature block
	BYTE *rawSigFinal;
	XSECnew(rawSigFinal, BYTE[rawSigLen]);
	ArrayJanitor<BYTE> j_rawSigFinal(rawSigFinal);

	BYTE * i, * j;

	i = rawSig;
	j = rawSigFinal + rawSigLen - 1;

	while (j >= rawSigFinal) {
		*j-- = *i++;
	}

	// Now encode
	XSCryptCryptoBase64 b64;
	b64.encodeInit();
	unsigned int ret = b64.encode(rawSigFinal, rawSigLen, (unsigned char *) base64SignatureBuf, base64SignatureBufLen);
	ret += b64.encodeFinish((unsigned char *) &base64SignatureBuf[ret], base64SignatureBufLen - ret);

	return ret;

}

XSECCryptoKey * NSSCryptoKeyRSA::clone() {

	NSSCryptoKeyRSA * ret;

	XSECnew(ret, NSSCryptoKeyRSA(m_p));
	
	if (m_key != 0) {

		// CryptDuplicateKey is not supported in Windows NT, so we need to export and then
		// reimport the key to get a copy

		BYTE keyBuf[2048];
		DWORD keyBufLen = 2048;
		CryptExportKey(m_key, 0, PUBLICKEYBLOB, 0, keyBuf, &keyBufLen);

		// Now re-import
		CryptImportKey(m_p, keyBuf, keyBufLen, NULL, 0, &ret->m_key);
	}

	ret->m_keySpec = m_keySpec;

	ret->m_exponentLen = m_exponentLen;
	if (mp_exponent != NULL) {
		XSECnew(ret->mp_exponent, BYTE[m_exponentLen]);
		memcpy(ret->mp_exponent, mp_exponent, m_exponentLen);
	}
	else
		ret->mp_exponent = NULL;

	ret->m_modulusLen = m_modulusLen;
	if (mp_modulus != NULL) {
		XSECnew(ret->mp_modulus, BYTE[m_modulusLen]);
		memcpy(ret->mp_modulus, mp_modulus, m_modulusLen);
	}
	else
		ret->mp_modulus = NULL;

	return ret;

}

// --------------------------------------------------------------------------------
//           decrypt a buffer
// --------------------------------------------------------------------------------

unsigned int NSSCryptoKeyRSA::privateDecrypt(const unsigned char * inBuf,
								 unsigned char * plainBuf, 
								 unsigned int inLength,
								 unsigned int maxOutLength,
								 PaddingType padding,
								 hashMethod hm) {

	// Perform a decrypt
	if (m_key == 0) {

		throw XSECCryptoException(XSECCryptoException::RSAError,
			"NSS:RSA - Attempt to decrypt data with empty key");
	}
	// Have to reverse ordering of input :
	DWORD decryptSize = inLength;
	memcpy(plainBuf, inBuf, inLength);
	for (unsigned int i = 0; i < inLength; ++i)
 		plainBuf[i] = inBuf[inLength - 1 - i];

	switch (padding) {

	case XSECCryptoKeyRSA::PAD_PKCS_1_5 :

		if (!CryptDecrypt(m_key,
						 0,
						 TRUE,
						 0,
						 plainBuf,
						 &decryptSize)) {

			//DWORD dw = GetLastError();

			throw XSECCryptoException(XSECCryptoException::RSAError,
				"NSS:RSA privateKeyDecrypt - Error Decrypting PKCS1_5 padded RSA encrypt");

		}

		break;

	case XSECCryptoKeyRSA::PAD_OAEP_MGFP1 :

		if (!CryptDecrypt(m_key,
						 0,
						 TRUE,
						 CRYPT_OAEP,
						 plainBuf,
						 &decryptSize)) {

			throw XSECCryptoException(XSECCryptoException::RSAError,
				"NSS:RSA privateKeyDecrypt - Error Decrypting PKCS1_5 padded RSA encrypt");

		}

		break;


	default :

		throw XSECCryptoException(XSECCryptoException::RSAError,
			"NSS:RSA - Unknown padding method");

	}


	return decryptSize;

}

// --------------------------------------------------------------------------------
//           encrypt a buffer
// --------------------------------------------------------------------------------

unsigned int NSSCryptoKeyRSA::publicEncrypt(const unsigned char * inBuf,
								 unsigned char * cipherBuf, 
								 unsigned int inLength,
								 unsigned int maxOutLength,
								 PaddingType padding,
								 hashMethod hm) {

	// Perform an encrypt
	if (m_key == 0) {

		throw XSECCryptoException(XSECCryptoException::RSAError,
			"NSS:RSA - Attempt to encrypt data with empty key");
	}

	DWORD encryptSize = inLength;
	memcpy(cipherBuf, inBuf, inLength);

	switch (padding) {

	case XSECCryptoKeyRSA::PAD_PKCS_1_5 :

		if (!CryptEncrypt(m_key,
						  0,			// No Hash
						  TRUE,	  // Is Final
						  0,			// No flags
						  cipherBuf,
						  &encryptSize,
						  maxOutLength)) {

			throw XSECCryptoException(XSECCryptoException::RSAError,
				"NSS:RSA publicKeyEncrypt - Error performing encrypt");
		}

		if (encryptSize <= 0) {

			throw XSECCryptoException(XSECCryptoException::RSAError,
				"NSS:RSA publicKeyEncrypt - Error performing PKCS1_5 padded RSA encrypt");

		}

		break;

	case XSECCryptoKeyRSA::PAD_OAEP_MGFP1 :

		if (!CryptEncrypt(m_key,
						  0,			  //No Hash
						  TRUE,			// Is Final
						  CRYPT_OAEP,
						  cipherBuf,
						  &encryptSize,
						  maxOutLength)) {

			throw XSECCryptoException(XSECCryptoException::RSAError,
				"NSS:RSA publicKeyEncrypt - Error performing encrypt");
		}

		if (encryptSize <= 0) {

			throw XSECCryptoException(XSECCryptoException::RSAError,
				"NSS:RSA publicKeyEncrypt - Error performing OAEP RSA encrypt");

		}

		break;

	default :

		throw XSECCryptoException(XSECCryptoException::RSAError,
			"NSS:RSA - Unknown padding method");

	}

	 // Reverse the output
   unsigned char *tbuf;
   XSECnew(tbuf, unsigned char[encryptSize]);
   ArrayJanitor<unsigned char> j_tbuf(tbuf);
   memcpy(tbuf, cipherBuf, encryptSize);
  	 
   for (unsigned int i = 0; i < encryptSize; ++i)
	   cipherBuf[i] = tbuf[encryptSize - 1 - i];


	return encryptSize;

}

// --------------------------------------------------------------------------------
//           Size in bytes
// --------------------------------------------------------------------------------

unsigned int NSSCryptoKeyRSA::getLength(void) {

	DWORD len;
	DWORD pLen = 4;

	if (!CryptGetKeyParam(m_key,
					 KP_BLOCKLEN,
					 (BYTE *) &len,
					 &pLen,
					 0)) {
		throw XSECCryptoException(XSECCryptoException::RSAError,
			"NSS:RSA - Error determining key size");
	}

	return len / 8;

}

// --------------------------------------------------------------------------------
//           Some utility functions
// --------------------------------------------------------------------------------

void NSSCryptoKeyRSA::loadParamsFromKey(void) {

	if (m_key == 0) {

		if (m_keySpec == 0)
			return;

		// See of we can get the user key
		if (!CryptGetUserKey(m_p, m_keySpec, &m_key))
			return;
	}

	// Export key into a keyblob
	BOOL fResult;
	DWORD blobLen;

	fResult = CryptExportKey(
		m_key,
		0,
		PUBLICKEYBLOB,
		0,
		NULL,
		&blobLen);
	
	if (fResult == 0 || blobLen < 1) {
		throw XSECCryptoException(XSECCryptoException::RSAError,
			"NSS:RSA - Error exporting public key");
	}

	BYTE * blob;
	XSECnew(blob, BYTE[blobLen]);
	ArrayJanitor<BYTE> j_blob(blob);

	fResult = CryptExportKey(
		m_key,
		0,
		PUBLICKEYBLOB,
		0,
		blob,
		&blobLen);
	
	if (fResult == 0 || blobLen < 1) {
		throw XSECCryptoException(XSECCryptoException::RSAError,
			"NSS:RSA - Error exporting public key");
	}

	RSAPUBKEY * pk = (RSAPUBKEY *) ( blob + NSS_BLOBHEADERLEN );
	DWORD keyLen = pk->bitlen / 8;

	// Copy the keys
	
	BYTE * i = (BYTE *) ( pk );
	i += NSS_RSAPUBKEYLEN;
	if (mp_modulus != NULL)
		delete[] mp_modulus;

	m_modulusLen = keyLen;
	XSECnew(mp_modulus, BYTE[m_modulusLen]);
	memcpy(mp_modulus, i, m_modulusLen);

	// Take the simple way out
	XSECnew(mp_exponent, BYTE[4]);
	*((DWORD *) mp_exponent) = pk->pubexp;

	// Now cut any leading 0s (Windows is LE, so start least significant end)

	m_exponentLen = 3;
	while (m_exponentLen > 0 && mp_exponent[m_exponentLen] == 0)
		m_exponentLen--;

	m_exponentLen++;	// Make it a length as apposed to an offset
}

unsigned int NSSCryptoKeyRSA::getExponentBase64BigNums(char * b64, unsigned int len) {

	if (m_key == 0 && m_keySpec == 0 && mp_exponent == NULL) {

		return 0;	// Nothing we can do

	}

	if (mp_exponent == NULL) {

		loadParamsFromKey();

	}

	unsigned int bLen;
	//unsigned char * b =  NSSCryptoProvider::WinBN2b64(mp_exponent, m_exponentLen, bLen);
  unsigned char * b;
	if (bLen > len)
		bLen = len;
	memcpy(b64, b, bLen);
	delete[] b;

	return bLen;

}

unsigned int NSSCryptoKeyRSA::getModulusBase64BigNums(char * b64, unsigned int len) {

	if (m_key == 0 && m_keySpec == 0 && mp_modulus == NULL) {

		return 0;	// Nothing we can do

	}

	if (mp_modulus == NULL) {

		loadParamsFromKey();

	}

	unsigned int bLen;
//	unsigned char * b =  NSSCryptoProvider::WinBN2b64(mp_modulus, m_modulusLen, bLen);
  unsigned char * b;
	if (bLen > len)
		bLen = len;
	memcpy(b64, b, bLen);
	delete[] b;

	return bLen;

}