/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */
#include <stdlib.h>
#include <al/al_random.h>
#include "ctr_drbg.h"

static int myrand( void *rng_state, unsigned char *output, size_t len )
{
    size_t use_len;
    int rnd;

    if( rng_state != NULL )
        rng_state  = NULL;

    while( len > 0 )
    {
        use_len = len;
        if( use_len > sizeof(int) )
            use_len = sizeof(int);

        rnd = rand();
        memcpy( output, &rnd, use_len );
        output += use_len;
        len -= use_len;
    }

    return( 0 );
}

int al_random_fill(void *buf, size_t len)
{
	char *p_buf = (char *)buf;
    // unsigned char randBuf[16];
    unsigned char ddh,ddl;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_ctr_drbg_init( &ctr_drbg );
    mbedtls_ctr_drbg_seed( &ctr_drbg, myrand, NULL, NULL, 0 );
    mbedtls_ctr_drbg_random( &ctr_drbg, p_buf, len);
    for(int i = 0; i < len/2; i++){
        ddh = 48 + p_buf[i] / 16;
        ddl = 48 + p_buf[i] % 16;
        if (ddh > 57) ddh = ddh + 39;
        if (ddl > 57) ddl = ddl + 39;
        p_buf[i*2] = ddh;
        p_buf[i*2+1] = ddl;
    }
    mbedtls_ctr_drbg_free( &ctr_drbg );
	return -1;	/* indicate no RNG */
}
