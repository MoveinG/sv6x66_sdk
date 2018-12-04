#include <string.h>
#include <stdio.h>
#include "compat-1.3.h"
#include "aes.h"
#include <malloc.h>
#include "osal/osal.h"
#include "base64.h"
#include "lwip/sockets.h"
#include "user_aes.h"




 
static unsigned char *AES_Padding_Zero(unsigned char *data,unsigned short data_len,unsigned short *padded_len)
{
    unsigned char len,padding_len;
    unsigned char *padded_data;

    len = data_len;
    padding_len = 16 - (len % 16);
    *padded_len = len + padding_len;
    padded_data = malloc(*padded_len);
	
    memset(padded_data,0,*padded_len);
    memcpy(padded_data,(const char*)data,len);
	
    return padded_data;
}


static size_t AES_Padding5(unsigned char *src, size_t srcLen)
{ 
	size_t paddNum = 16 - srcLen % 16; 
	for (int i = 0; i < paddNum; i++) 
	{ 
		src[srcLen + i] = paddNum; 
	}
	return srcLen + paddNum; 
}




void user_aes_encrypt(unsigned char *indata, unsigned char *outdata)
{
	
	static unsigned char cypher[64];
	unsigned char *iplain = NULL;
	unsigned char *plain = NULL;
	unsigned short padlen;
	char length;
	size_t enlen;
	mbedtls_aes_context aes;
	unsigned char dst[128];
	unsigned char IV[20] = "0102030405060708";
	unsigned char key[20] = "KEYDIY2018szecar";
	
	length = strlen((char *)indata); 
	iplain = indata;
	
	mbedtls_aes_init( &aes );
	//plain = AES_Padding_Zero(iplain, length, &padlen);
	padlen = AES_Padding5(iplain,length);
	plain = iplain;
	mbedtls_aes_setkey_enc(&aes, key, 128);	
	mbedtls_aes_crypt_cbc(&aes, AES_ENCRYPT, padlen, IV, plain, cypher);\
	mbedtls_base64_encode(dst, sizeof(dst), &enlen, cypher, padlen);
	
	length = strlen((char *)dst);
    //free(plain);
	plain = NULL;
	memcpy(outdata, dst, enlen);

}


void user_aes_decrypt(unsigned char *indata, unsigned char *outdata)
{
	
	static unsigned char plain_decrypt[64];
	unsigned char rst[128];
	size_t delen;
	unsigned short padlen;
	unsigned char *iplain = NULL;
	unsigned char *plain = NULL;
	char length;
	mbedtls_aes_context aes;
	unsigned char IV[20] = "0102030405060708";
	unsigned char key[20] = "KEYDIY2018szecar";
	
	length = strlen((char *)indata);
	iplain = indata;
	mbedtls_aes_init( &aes );
	mbedtls_base64_decode(rst, sizeof(rst), &delen, iplain, length);
	//plain = AES_Padding_Zero(rst, delen, &padlen);
	padlen = AES_Padding5(rst,length);
	plain = iplain;
	mbedtls_aes_setkey_dec(&aes, key, 128);
	mbedtls_aes_crypt_cbc(&aes, AES_DECRYPT, padlen, IV, rst, plain_decrypt);
	memcpy(outdata, plain_decrypt, padlen);

}
					


