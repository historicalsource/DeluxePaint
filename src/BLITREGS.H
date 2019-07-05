
typedef struct {
    USHORT	bltcon0,bltcon1,fwmask,lwmask;
    UBYTE	*bltptc,*bltptb,*bltpta,*bltptd;
    USHORT	bltsize;
    SHORT	ioskip2[3];
    SHORT	bltmdc,bltmdb,bltmda,bltmdd;
    SHORT	ioskip3[4];
    USHORT	cdata,bdata,adata;
    } BlitterRegs;
    
BlitterRegs *ar = (BlitterRegs *)0xDFF040;

struct bltnode myNode;

#define USEA 0x0800
#define USEB 0x0400
#define USEC 0x0200
#define USED 0x0100
#define MASKWRITE  	0x0F00  /* USEA|USEB|USEC|USED */
#define CONSTWRITE	0x0B00	/* USEA|USEC|USED */
    
#define COPYA 0xf0	/* minterm = A */
