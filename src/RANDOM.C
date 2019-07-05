/*--------------------------------------------------------------*/
/*								*/
/*			random.c				*/
/*								*/
/*--------------------------------------------------------------*/
#include <exec\types.h>

/* ---- Random Number Generator ----- */
    
static SHORT rvect[32] = {
    0x3d52,0x7f31,0x80a2,0xed16,0x42bf,0x5c41,0x0c5e,0x9226,
    0xa58b,0x482d,0x64d9,0x82ad,0xe179,0x214b,0x814c,0xd736,
    0x97cf,0xaeb1,0xf7cb,0x6a9b,0x0c59,0x8534,0x7189,0x1593,
    0x6bc5,0x07ac,0x3c7f,0xa43e,0xda69,0x181e,0x9bd1,0xd57f 
    };

static randi1=0;  
static randi2=15;

/* dont need to call this unless want to get different series of nums */
InitRand(seed) SHORT seed; { int i; for (i=0; i<32; i++) rvect[i] += seed;  }

/* Return a 16 bit random bit pattern */
SHORT Rand16() { 
    SHORT res = rvect[randi1] = rvect[randi1] + rvect[randi2];
    randi1 = (randi1+1)&31;   randi2 = (randi2+1)&31;
    return(res);
    }

/* Returns a 15 bit random number in range [0..n-1] */

SHORT Random(n) int n; { return((SHORT)((Rand16()&0x7fff)%n)); }
    
static SHORT dummy;

