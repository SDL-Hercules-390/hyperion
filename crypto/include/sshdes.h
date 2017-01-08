#ifndef _SSHDES_H
#define _SSHDES_H

/* The following three functions were declared 'static' in sshdes.c,
 * but are being made public instead as there may be legitimate need
 * to call them directly from user code. The DESContext struct was
 * also defined in sshdes.c and was moved here for obvious reasons.
 */
typedef u_int32_t  word32;

typedef struct {
    word32 k0246[16], k1357[16];
    word32 iv0, iv1;
} DESContext;

void des_key_setup ( word32 key_msw, word32 key_lsw,     DESContext* sched );
void des_encipher  ( word32* output, word32 L, word32 R, DESContext* sched );
void des_decipher  ( word32* output, word32 L, word32 R, DESContext* sched );

#endif /*_SSHDES_H*/
