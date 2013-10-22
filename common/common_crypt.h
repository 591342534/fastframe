/*
 * common_crypt.h
 *
 *  Created on: 2012-12-10
 *      Author: jimm
 */

#ifndef COMMON_CRYPT_H_
#define COMMON_CRYPT_H_

#include <stdint.h>

#define ENCRYPT_BLOCK_LENGTH_IN_BYTE (8)

class CXTEA
{
public:
	static int32_t Encrypt(char* pbyInBuffer, int32_t nInBufferLength, char* pbyOutBuffer, int32_t nOutBufferLength, char arrbyKey[16]);
	static int32_t Decrypt(char* pbyInBuffer, int32_t nInBufferLength, char* pbyOutBuffer, int32_t nOutBufferLength, char arrbyKey[16]);
};

#endif /* COMMON_CRYPT_H_ */
